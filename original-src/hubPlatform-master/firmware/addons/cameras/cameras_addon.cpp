#include "cameras/cameras_addon.h"
#include "cameras/common_res.h"

#include "cameras/rtsp_server/rtsp_server_exception.h"
#include "cameras/rtsp_server/live555/rtsp_server_controller.h"
#include "cameras/rtsp_server/camera_framed_source_options.h"

#include "cameras/events/live_rtsp_stream.h"
#include "cameras/events/stream_snapshot_events.h"
#include "cameras/events/particle_recording_events.h"
#include "cameras/events/event_publisher_impl.h"
#include "cameras/events/camera_diagnostic_events.h"

#include "cameras/camera.h"
#include "cameras/version/cameras_addon_version.h"
#include "cameras/storages/storage.h"
#include "cameras/packet/packet_publisher.h"

#include "cameras/input_output/camera_output_options.h"
#include "cameras/input_output/camera_snapshoter_options.h"
#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_options.h"

#include "cameras/postponed/postponed_method_manager.h"
#include "cameras/local_stream/live_streamer.h"
#include "cameras/cloud/cloud_stream_saver.h"
#include "cameras/cloud/stream_particle_recorder.h"
#include "cameras/snapshot/snapshot_manager.h"

#include "cameras/stream_snapshot_exception.h"

#include "cloud_storage/cloud_storage_utils.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "base_utils/utils.h"
#include "jsonrpc/jsonrpc.h"
#include "json_utils/json_utils.h"
#include "config_reader/config_keys.h"
#include "config_reader/config_reader.h"

#include "system_utils/filesystem.h"
#include "system_utils/environment.h"

#include "networkd/remote_interface/network_remote_interface.h"
#include "networkd/resources/nm_public_resources.h"
#include "networkd/ssh_tunnel/ssh_defs_resources.h"

#include "cored/remote_interface/cored_interface_factory.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"

#include <curl/curl.h>
#include <chrono>

namespace HubPlatform {
namespace Cameras {

constexpr std::chrono::hours CamerasAddon::StreamStatusesPrintPeriod;
constexpr std::chrono::minutes CamerasAddon::StreamStatusesFirstPrintTime;

namespace {
class CamerasAddonException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string tryGetNonEmptyString( const JsonObject& json, const std::string& key ) {
    const std::string result = json_get_str( json, key );
    if( result.empty() ) {
        throw CamerasAddonException( "Missing key \'" + key + "\" in json" );
    }
    return result;
}

int tryGetInt( const JsonObject& json, const std::string& key ) {
    const int result = json_get_int( json, key );
    if( result < 0 ) {
        throw CamerasAddonException( "Missing key \'" + key + "\" in json" );
    }
    return result;
}

template < class T >
std::unique_lock< T > tryCreateUniqueLock( T& mutex, std::chrono::milliseconds timeout = std::chrono::seconds{ 60 } ) {
    std::unique_lock< T > lock( mutex, std::defer_lock );
    if( !waitForCondition( [&lock] { return lock.try_lock(); }, timeout, std::chrono::milliseconds{ 10 } ) ) {
        LOGGER_ERROR( "Deadlock detected in camera addon!!! Stalled thread: " + cc::threadId() );
        throw CamerasAddonException( "Deadlock in camera addon detected!!! Stalled thread: " + cc::threadId() );
    }
    return lock;
}

void ffmpegLog( void*, int level, const char* szFmt, va_list varg ) {
    const auto module = "ffmpeg";
    switch( level ) {
    case AV_LOG_TRACE:
    case AV_LOG_VERBOSE:
        break;

    case AV_LOG_DEBUG:
        break;

    case AV_LOG_INFO:
        Log::printList( Log::PriorityLevel::INFO, szFmt, varg, module );
        break;

    case AV_LOG_ERROR:
        Log::printList( Log::PriorityLevel::ERROR, szFmt, varg, module );
        break;

    case AV_LOG_WARNING:
        Log::printList( Log::PriorityLevel::WARN, szFmt, varg, module );
        break;

    case AV_LOG_FATAL:
    case AV_LOG_PANIC:
        Log::printList( Log::PriorityLevel::FATAL, szFmt, varg, module );
        break;

    default:
        Log::printList( Log::PriorityLevel::INFO, szFmt, varg, module );
        break;
    }
}

}  // namespace

CamerasAddon::CamerasAddon()
: Addons::Addon( CamerasAddonName ) {
    curl_global_init( CURL_GLOBAL_ALL );

    av_log_set_callback( ffmpegLog );
    applyNewLogLevel( Log::getLogLevel() );

    filesystem::createFolder( Environment::getVideosFolder() );
    filesystem::createFolder( Environment::getSnapshotsFolder() );
    filesystem::clearFolder( Environment::getVideosFolder() );
    filesystem::clearFolder( Environment::getSnapshotsFolder() );
}

CamerasAddon::~CamerasAddon() {
    curl_global_cleanup();
    m_imageRemovers.clear();
}

CommonUtils::AddonVersion CamerasAddon::getVersion() const {
    return Addons::Cameras::getVersion();
}

void CamerasAddon::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleEventFromBus( message );
        break;

    case LOG_CMD:
        handleLogCommand( message, sender );
        applyNewLogLevel( Log::getLogLevel() );
        break;

    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void CamerasAddon::setUp() {
    Addon::setUp();
    applyNewLogLevel( Log::getLogLevel() );

    join( GroupsNames::BROADCAST_GROUP );

    m_cloudStorageInterface = std::make_unique< Common::CloudStorageInterface::RemoteInterface >( getDBClient() );
    m_networkRemoteInterface = std::make_unique< Network::RemoteInterface >( *this );

    ConfigReader configReader;

    setConfigIntOption( configReader,
                        []( int v ) { ILocalStorage::setMaxStorageVolume( v ); },
                        ConfigKeys::CamerasLocalStorageMaxSizeMb,
                        DefaultLocalStorageMaxSizeMb );

    setConfigIntOption( configReader,
                        [this]( int v ) { m_rtspServerPort = v; },
                        ConfigKeys::CamerasRtspServerPort,
                        DefaultRtspServerPort );
    setConfigIntOption( configReader,
                        []( int v ) { CameraFramedSourceOptions::setBufferSize( v ); },
                        ConfigKeys::CamerasRtspServerPacketBufferSize,
                        DefaultRtspServerPacketBufferSize );

    setConfigIntOption( configReader,
                        []( int v ) { CameraInputOptions::setMaxDelayBetweenPacketsMsec( v ); },
                        ConfigKeys::CamerasStreamingMaxDelayMsec,
                        DefaultStreamingMaxDelayMsec );

    setConfigIntOption( configReader,
                        []( int v ) { CameraOutputOptions::setRecordBufferSize( v ); },
                        ConfigKeys::CamerasRecordingBufferSize,
                        DefaultRecordingBufferSize );
    setConfigIntOption( configReader,
                        []( int v ) { CameraOutputOptions::setPreRecordBufferSize( v ); },
                        ConfigKeys::CamerasPreRecordingBufferSize,
                        DefaultPreRecordingBufferSize );

    setConfigIntOption( configReader,
                        []( int v ) { CameraSnapshoterOptions::setBufferSize( v ); },
                        ConfigKeys::CamerasSnapshoterBufferSize,
                        DefaultSnapshoterBufferSize );
    setConfigIntOption( configReader,
                        []( int v ) { CameraSnapshoterOptions::setQueueLimit( v ); },
                        ConfigKeys::CamerasSnapshoterQueueLimit,
                        DefaultSnapshoterQueueLimit );
    setConfigIntOption( configReader,
                        []( int v ) { CameraSnapshoterOptions::setTimeoutSec( v ); },
                        ConfigKeys::CamerasSnapshoterTimeout,
                        DefaultSnapshoterTimeout );

    setConfigIntOption( configReader,
                        [this]( int v ) { m_makeSnapshotTimeout = v; },
                        ConfigKeys::CamerasSnapshotCacheTimeout,
                        DefaultSnapshotTimeout );

    CameraInputOptions::setPacketProcessingDelayMsec( 0 );
    CameraInputOptions::setAccumulatedDelayThresholdMsec( DefaultAccumulatedDelayThresholdMsec );

    Json networkInterfaces;
    do {
        networkInterfaces = m_networkRemoteInterface->getLocalNetworkInterface();
        if( networkInterfaces.empty() ) {
            Log::warn( cc::warn( "Empty network interfaces" ) );
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
        else {
            break;
        }
    } while( true );

    updateIpAddress( networkInterfaces );
    readConnectedNetworkInterfaces( m_networkRemoteInterface->getConnectedNetworkInterfaces() );

    m_serverEventPublisher = std::make_unique< EventPublisher< EventBase > >();

    do {
        try {
            Log::debug( cc::debug( "Creating rtsp server ..." ) );
            m_rtspServer = std::make_unique< Live555::RtspServerController >(
                m_rtspServerPort, CameraFramedSourceOptions::getBufferSize(), *m_serverEventPublisher );
            m_rtspServer->start();
            m_rtspServer->waitForStart( std::chrono::seconds( 10 ) );
            break;
        }
        catch( const std::exception& ex ) {
            Log::warn( cc::warn( std::string( "Failed to setup rtsp server: " ) + ex.what() ) );
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
    } while( true );
    Log::info( cc::info( "Rtsp server started on port " ) + cc::note( std::to_string( m_rtspServerPort ) ) );

    startLiveStreams();

    m_snapshotCache = std::make_unique< SnapshotCache >( this->getDBClient(), m_makeSnapshotTimeout );

    m_streamStatusPrinter = createPostponedTask( [this]( Services::PostponedTask& ) { printStreamStatuses(); } );
    m_streamStatusPrinter->start( StreamStatusesFirstPrintTime, StreamStatusesPrintPeriod );

    m_cachedSnapshotsProcessing = createPostponedTask( [this]( Services::PostponedTask& ) {
        m_snapshotCache->tryToClearNotProcessedImages(
            [this]( const std::string& streamId ) { removeCachedImageIfExpired( streamId ); } );

        m_snapshotCache->requestsProcessing(
            [this]( const Json& params ) { makeStreamSnapshotForCloudInternal( params ); } );
    } );

    m_cachedSnapshotsProcessing->start( std::chrono::seconds( 3 ), true );
}

void CamerasAddon::tearDown() {
    m_streamStatusPrinter->stop();
    m_cachedSnapshotsProcessing->stop();
    m_rtspServer.reset();
    m_cameras.clear();
    m_pluginIds.clear();
    m_serverEventPublisher.reset();
    m_networkRemoteInterface.reset();
    m_cloudStorageInterface.reset();
    m_imageRemovers.clear();
    Addon::tearDown();
}

void CamerasAddon::onRTSPStreamStarted( const StreamId& streamId, const LiveRTSPStreamLink& rtspLink ) {
    auto link = updateLiveRtspStreamLink( rtspLink );
    sendBroadcast( CAMERAS_BROADCAST_START_RTSP_STREAMING,
                   JSON( "streamId" << streamId << "localRtspUrl" << link ),
                   GroupsNames::CAMERAS_BROADCASTS );
    emitEventToPlugin( streamId, LiveRtspStreamStarted( streamId, link ) );

    std::lock_guard< std::mutex > lock( m_streamStatusMutex );
    if( auto status = findStreamStatus( streamId ) ) {
        status->controllerUrl = rtspLink;
    }

    Log::info( cc::info( "Created live RTSP stream " ) + cc::note( streamId ) + cc::info( " with URL " ) +
               cc::u( link ) );
}

void CamerasAddon::onRTSPStreamStopped( const StreamId& streamId ) {
    sendBroadcast(
        CAMERAS_BROADCAST_STOP_RTSP_STREAMING, JSON( "streamId" << streamId ), GroupsNames::CAMERAS_BROADCASTS );
    emitEventToPlugin( streamId, LiveRtspStreamStopped( streamId ) );
    emitEventToPlugin( streamId, CameraStreamClosed( streamId ) );

    Log::info( cc::info( "Deleted live RTSP stream: " ) + cc::note( streamId ) );
}

void CamerasAddon::onRTSPStreamCreationFailed( const StreamId& streamId, const std::string& errorMessage ) {
    emitEventToPlugin( streamId, LiveRtspStreamError( streamId, errorMessage ) );

    std::lock_guard< std::mutex > lock( m_streamStatusMutex );
    if( auto status = findStreamStatus( streamId ) ) {
        status->lastStreamCreatingError = errorMessage;
    }

    Log::error( cc::error( "RTSP stream " ) + cc::note( streamId ) + cc::error( ", creating error " ) +
                cc::attention( errorMessage ) );
}

void CamerasAddon::onRTSPStreamError( const CamerasAddon::StreamId& streamId, StreamError error ) {
    switch( error ) {
    case StreamError::CAMERA_STREAM_CLOSED:
        emitEventToPlugin( streamId, CameraStreamClosed( streamId ) );
        break;

    case StreamError::CAMERA_PACKET_DELAY:
        emitEventToPlugin( streamId, CameraPacketDelay( streamId ) );
        break;

    case StreamError::CAMERA_ACCUMULATED_DELAY:
        emitEventToPlugin( streamId, CameraAccumulatedDelay( streamId ) );
        break;

    case StreamError::CAMERA_UNREACHABLE:
        emitEventToPlugin( streamId, CameraUnreachable( streamId ) );
        break;
    }
}

CameraPtr CamerasAddon::createCamera( const std::string& streamId, const std::string& rtspUrl ) {
    auto cameraInput = std::make_unique< CameraInput >( std::make_shared< PacketHandler >(), rtspUrl );
    auto postponedMethodManager = std::make_unique< PostponedMethodManager >();
    auto liveStreamer = std::make_unique< LiveStreamer >( *m_rtspServer, *cameraInput, streamId );
    auto cloudStreamSaver =
        std::make_unique< CloudStreamSaver >( *cameraInput, streamId, Environment::getVideosFolder() );
    auto particleRecorder =
        std::make_unique< StreamParticleRecorder >( *cameraInput, streamId, Environment::getVideosFolder() );
    auto snapshotManager =
        std::make_unique< SnapshotManager >( streamId, *cameraInput, Environment::getSnapshotsFolder() );

    filesystem::createFolder( Environment::getVideosFolder() );
    filesystem::createFolder( Environment::getSnapshotsFolder() );

    auto camera = std::make_unique< Camera >( streamId,
                                              m_currentIpAddress + ":" + std::to_string( m_rtspServerPort ),
                                              std::move( cameraInput ),
                                              std::move( postponedMethodManager ),
                                              std::move( liveStreamer ),
                                              std::move( cloudStreamSaver ),
                                              std::move( particleRecorder ),
                                              std::move( snapshotManager ),
                                              *this );
    camera->startStream();
    return camera;
}

JsonObject CamerasAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    JsonObject answer;
    try {
        switch( requestType ) {
        case CAMERAS_ADD_STREAM:
            LOGGER_INFO_RAW( cc::info( "Addon: Got request to add stream: " ) +
                             cc::note( json_get_str( request, "streamId" ) ) );
            addCameraStream( request );
            break;
        case CAMERAS_REMOVE_STREAM:
            LOGGER_INFO_RAW( cc::info( "Addon: Got request to remove stream: " ) +
                             cc::note( json_get_str( request, "streamId" ) ) );
            removeCameraStream( request );
            break;
        case CAMERAS_GET_STREAM_INFO:
            return getStreamInfo( request );
        case REQUEST_CAMERA_STREAMS:
            return getCameraStreams();
        case GLOBAL_LOCK: {
            auto& lockHelper = CommonUtils::LockHelper::global();
            lockHelper.tryLock( request );
            return lockHelper.getResponse();
        }
        case GLOBAL_UNLOCK: {
            auto& lockHelper = CommonUtils::LockHelper::global();
            lockHelper.unlock( request );
            return lockHelper.getResponse();
        }
        case BACKUP_CREATE: {
            CommonUtils::BackupHelper helper( getDBClient(), request );
            helper.backupDatabase( CameraStreamsDBName );
            return helper.getResponse();
        }
        default:
            return Service::handleRequestFromBus( request, requestType );
        }
    }
    catch( const std::exception& ex ) {
        const std::string reason = ex.what();
        Log::error( cc::error( "request error: " + cc::attention( reason ) ) );
        setError( answer, Errors::WRONG_PARAMS, reason );
    }
    return answer;
}

int CamerasAddon::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    helper.restoreDatabase( CameraStreamsDBName );
    return helper.getReturnCode();
}

void CamerasAddon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    Addon::handleLegacyMethodCall( json, sender );
}

void CamerasAddon::handleMethodCall( const JsonObject& json,
                                     MessageSubClasses requestType,
                                     const std::string& /*sender*/ ) {
    using MethodHandler = void ( CamerasAddon::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { CAMERAS_CREATE_LIVE_RTSP_STREAM, &CamerasAddon::createLiveRtspStream },
        { CAMERAS_DELETE_LIVE_RTSP_STREAM, &CamerasAddon::deleteLiveRtspStream },
        { CAMERAS_CREATE_STREAM_SAVING_TO_CLOUD, &CamerasAddon::createStreamSavingToCloud },
        { CAMERAS_DELETE_STREAM_SAVING_TO_CLOUD, &CamerasAddon::deleteStreamSavingToCloud },
        { CAMERAS_START_STREAM_SAVING_TO_CLOUD, &CamerasAddon::startStreamSavingToCloud },
        { CAMERAS_STOP_STREAM_SAVING_TO_CLOUD, &CamerasAddon::stopStreamSavingToCloud },
        { CAMERAS_SET_VIDEO_PARTICLE_SETTINGS, &CamerasAddon::setVideoParticleSettings },
        { CAMERAS_MAKE_SNAPSHOT_FOR_CLOUD, &CamerasAddon::makeStreamSnapshotForCloud },
        { EXTERNAL_EVENT, &CamerasAddon::handleExternalEvent },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        try {
            LOGGER_INFO_RAW( cc::info( "Addon: Got RPC message: " ) +
                             cc::note( msgSubclassToStr( JSON_RPC, requestType ) ) );
            ( this->*handler )( json );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( ex.what() );
        }
    }
}

bool CamerasAddon::handleAsyncRequestFromBus( const JsonObject& request,
                                              MessageSubClasses requestType,
                                              const std::string& uuid,
                                              const std::string& sender ) {
    switch( requestType ) {
    case CAMERAS_MAKE_SNAPSHOT_FOR_UI:
        makeStreamSnapshotForUI( request, uuid, sender );
        break;
    default:
        return Service::handleAsyncRequestFromBus( request, requestType, uuid, sender );
    }
    return true;
}

void CamerasAddon::addCameraStream( const JsonObject& params ) {
    Log::debug( cc::debug( "Addon: Adding camera stream: " ) + cc::j( params ) );
    const std::string pluginId = json_get_str( params, "pluginId" );
    const std::string streamId = tryGetNonEmptyString( params, "streamId" );
    const std::string streamName = tryGetNonEmptyString( params, "streamName" );
    const std::string rtspUrl = tryGetNonEmptyString( params, "rtspUrl" );
    const std::string description = json_get_str( params, "description" );
    // Check if unique.
    const JsonObject existingStreams =
        getDBClient().findAll( CameraStreamsDBName, JSON( "streamId" << streamId ), nullptr );
    if( !existingStreams.empty() ) {
        throw CamerasAddonException( "Stream with id \"" + streamId + "\" already exists" );
    }

    JsonObject newCamera = JSON( "pluginId" << pluginId << "streamId" << streamId << "streamName" << streamName
                                            << "rtspUrl" << rtspUrl << "description" << params[ "description" ] );
    getDBClient().insert( CameraStreamsDBName, newCamera );

    {
        std::lock_guard< std::mutex > lock( m_pluginIdsMutex );
        m_pluginIds.emplace( streamId, pluginId );
    }

    {
        std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
        m_streamStatuses.emplace( streamId, StreamStatus( pluginId, streamName, description, rtspUrl, Clock::now() ) );
    }

    m_cameras.emplace( streamId, createCamera( streamId, rtspUrl ) );

    Log::info( cc::info( "Added camera stream " ) + cc::note( streamId ) );
}

void CamerasAddon::removeCameraStream( const JsonObject& params ) {
    LOGGER_DEBUG_RAW( cc::debug( "Addon: Removing camera stream: " ) + cc::j( params ) );
    const std::string streamId = tryGetNonEmptyString( params, "streamId" );

    auto lock = tryCreateUniqueLock( m_mutex );
    deleteLiveRtspStream( params );
    deleteStreamSavingToCloud( streamId );
    const auto joQuery = JSON( "streamId" << streamId );
    getDBClient().remove( CameraStreamsDBName, joQuery );
    m_cameras.erase( streamId );

    std::unique_lock< std::mutex > statusLock( m_streamStatusMutex );
    m_streamStatuses.erase( streamId );
    if( auto streamStatusPtr = findStreamStatus( streamId ) ) {
        streamStatusPtr->status = StreamStatus::Status::Removed;
        streamStatusPtr->lastRemoveDate = Clock::now();
    }
    statusLock.unlock();

    LOGGER_DEBUG_RAW( cc::info( "Removed camera stream " ) + cc::note( streamId ) );
    lock.unlock();
    sendBroadcast( CAMERAS_BROADCAST_REMOVE_STREAM, JSON( "streamId" << streamId ), GroupsNames::CAMERAS_BROADCASTS );
}

void CamerasAddon::createLiveRtspStream( const JsonObject& params ) {
    Log::debug( cc::debug( "Addon: creating live stream: " ) + cc::j( params ) );

    std::string streamId;
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        streamId = tryGetNonEmptyString( params, "streamId" );
        auto& camera = findOrCreateCamera( streamId );

        camera.createLiveRtspStream();

        getDBClient().update( CameraStreamsDBName, JSON( "streamId" << streamId ), []( Json& data ) {
            data[ "rtspStreamStatus" ] = "running";
        } );

        std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
        if( auto streamStatusPtr = findStreamStatus( streamId ) ) {
            streamStatusPtr->status = StreamStatus::Status::Started;
            streamStatusPtr->lastStartDate = Clock::now();
        }
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( "Create RTSP stream exception: " ) + cc::attention( ex.what() ) );
        if( !streamId.empty() ) {
            emitEventToPlugin( streamId, LiveRtspStreamError( ex.what() ) );
        }
    }
}

void CamerasAddon::deleteLiveRtspStream( const JsonObject& params ) {
    Log::debug( cc::debug( "Addon: Deleting live stream: " ) + cc::j( params ) );

    std::string streamId;
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        streamId = tryGetNonEmptyString( params, "streamId" );
        auto it = m_cameras.find( streamId );
        if( it == m_cameras.end() ) {
            return;
        }
        it->second->deleteLiveRtspStream();

        std::unique_lock< std::mutex > statusLock( m_streamStatusMutex );
        if( auto streamStatusPtr = findStreamStatus( streamId ) ) {
            streamStatusPtr->status = StreamStatus::Status::Stopped;
            streamStatusPtr->lastStopDate = Clock::now();
        }
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Delete RTSP stream exception: " ) ) + cc::attention( ex.what() ) );
        if( !streamId.empty() ) {
            emitEventToPlugin( streamId, LiveRtspStreamError( streamId, ex.what() ) );
        }
    }
    getDBClient().update( CameraStreamsDBName, JSON( "streamId" << streamId ), []( Json& data ) {
        data[ "rtspStreamStatus" ] = "stopped";
    } );
}

void CamerasAddon::createStreamSavingToCloud( const JsonObject& params ) {
    Log::debug( cc::debug( "Addon: Creating stream saving to cloud: " ) + cc::j( params ) );
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        const std::string streamId = tryGetNonEmptyString( params, "streamId" );
        auto& camera = findOrCreateCamera( streamId );

        const int secondsBeforeStart = tryGetInt( params, "secondsBeforeStart" );
        const int segmentDurationSec = tryGetInt( params, "segmentDurationSec" );
        const int segmentsLimit = tryGetInt( params, "segmentsLimit" );
        const auto format = json_get_str( params, "format" );

        using namespace std::placeholders;
        auto saveRecordingCallback = std::bind( &CamerasAddon::saveRecodingToCloud, this, streamId, _1, _2, _3, _4 );
        auto saveParticleCallback = std::bind( &CamerasAddon::saveParticleToCloud, this, streamId, _1, _2, _3, _4 );

        camera.createStreamSavingToCloud(
            segmentDurationSec, secondsBeforeStart, segmentsLimit, format, saveRecordingCallback );
        Log::info( cc::info( "Create stream saving to cloud, streamId: " ) + cc::note( streamId ) );

        const StreamParticleSettings particleSettings = findParticleSettings( streamId );
        camera.createStreamParticleRecording( particleSettings, saveParticleCallback );
        Log::info( cc::info( "Created particle recording, streamId: " ) + cc::note( streamId ) );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Stream saving to cloud exception: " ) ) + cc::attention( ex.what() ) );
    }
}

void CamerasAddon::deleteStreamSavingToCloud( const JsonObject& params ) {
    Log::debug( cc::debug( "Addon: Deleting stream saving to cloud: " ) + cc::j( params ) );
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        const std::string streamId = tryGetNonEmptyString( params, "streamId" );
        auto it = m_cameras.find( streamId );
        if( it == m_cameras.end() ) {
            LOGGER_DEBUG( cc::debug( "Deleting stream saving to cloud skipped, stream is not running, streamId: " ) +
                          cc::note( streamId ) );
            return;
        }

        auto& camera = *( it->second );
        camera.deleteStreamSavingToCloud();
        Log::info( cc::info( "Deleted stream saving to cloud, streamId: " ) + cc::note( streamId ) );

        camera.deleteStreamParticleRecording();
        Log::info( cc::info( "Deleted particle recording, streamId: " ) + cc::note( streamId ) );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Stream deleting to cloud exception: " ) ) + cc::attention( ex.what() ) );
    }
}

void CamerasAddon::startStreamSavingToCloud( const JsonObject& params ) {
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        const std::string streamId = tryGetNonEmptyString( params, "streamId" );
        auto& camera = findOrCreateCamera( streamId );
        Json metaData;
        json_get( params, "metaData", metaData );
        const bool shouldMakePreviewRecord = json_get_bool( params, "makeVideoParticle", false );

        Log::info( cc::info( "Addon: Starting stream saving to cloud, streamId: " ) + cc::note( streamId ) );
        using namespace std::placeholders;
        camera.startStreamSavingToCloud(
            metaData, std::bind( &CamerasAddon::saveRecodingToCloud, this, streamId, _1, _2, _3, _4 ) );

        if( shouldMakePreviewRecord ) {
            makePreviewParticle( camera, streamId );
        }
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Stream starting to cloud exception: " ) ) + cc::attention( ex.what() ) );
    }
}

void CamerasAddon::stopStreamSavingToCloud( const JsonObject& params ) {
    Log::debug( cc::debug( "Addon: Stopping stream saving to cloud: " ) + cc::j( params ) );
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        const std::string streamId = tryGetNonEmptyString( params, "streamId" );
        auto it = m_cameras.find( streamId );
        if( it == m_cameras.end() ) {
            LOGGER_DEBUG( cc::debug( "Stopping stream saving to cloud skipped, stream is not running, streamId: " ) +
                          cc::note( streamId ) );
            return;
        }

        auto& camera = *( it->second );
        camera.stopStreamSavingToCloud();
        camera.stopStreamParticleRecording();
        Log::info( cc::info( "Stopped stream saving to cloud, streamId: " ) + cc::note( streamId ) );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Stream stopping to cloud exception: " ) ) + cc::attention( ex.what() ) );
    }
}

void CamerasAddon::setVideoParticleSettings( const JsonObject& params ) {
    Log::debug( cc::debug( "Set preview particle settings: " ) + cc::j( params ) );
    try {
        const PluginId pluginId = tryGetNonEmptyString( params, "pluginId" );
        const int preRecordDurationSec = tryGetInt( params, "preRecordDurationSec" );
        const int recordDurationSec = tryGetInt( params, "recordDurationSec" );
        int resolutionDivisor = json_get_int( params, "resolutionDivisor", MinParticleResolutionDivisor );
        std::string format = json_get_str( params, "format" );

        if( resolutionDivisor < MinParticleResolutionDivisor ) {
            Log::warn( cc::warn( "Particle's resolution divisor is too low, re-setting its value to default" ) );
            resolutionDivisor = MinParticleResolutionDivisor;
        }

        StreamParticleSettings& settings = m_particlesSettings[ pluginId ];
        settings.preRecordDuration = std::chrono::seconds( preRecordDurationSec );
        settings.recordDuration = std::chrono::seconds( recordDurationSec );
        settings.resolutionDivisor = static_cast< uint32_t >( resolutionDivisor );
        settings.format = std::move( format );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Set preview particle settings exception: " ) ) +
                    cc::attention( ex.what() ) );
    }
}

void CamerasAddon::makePreviewParticle( Camera& camera, const StreamId& streamId ) {
    auto lock = tryCreateUniqueLock( m_mutex );
    if( !isParticleRecordingsLimitReached() ) {
        camera.recordStreamParticle( Json() );
        Log::info( cc::info( "Making stream preview particle, streamId: " ) + cc::note( streamId ) );
    }
    else {
        const std::string message = "Reached limit of simultaneous particle recordings";
        Log::warn( cc::warn( message ) );
        emitEventToPlugin( streamId, MakingVideoParticleFailed( streamId, message ) );
        std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
        if( auto status = findStreamStatus( streamId ) ) {
            status->lastPreviewParticleCreatingError = message;
        }
    }
}

void CamerasAddon::makeStreamSnapshotForUI( const JsonObject& params,
                                            const std::string& uuid,
                                            const std::string& sender ) {
    auto requestId = tryGetNonEmptyString( params, "requestId" );
    Log::debug( cc::debug( "Make stream snapshot for UI: " ) + cc::j( params ) );
    const int fixedImageWidth = 256;
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string streamId;

    auto lock = tryCreateUniqueLock( m_mutex );
    try {
        streamId = tryGetNonEmptyString( params, "streamId" );
        auto it = m_cameras.find( streamId );
        if( it == m_cameras.end() ) {
            throw StreamSnapshotException( "Stream is not created: " + streamId + ", requestId: " + requestId );
        }
        auto& camera = *it->second;
        if( !camera.isRunning() ) {
            throw StreamSnapshotException( "Stream is not running: " + streamId + ", requestId: " + requestId );
        }
        auto resolution = std::make_shared< Resolution >( fixedImageWidth, 0 );
        camera.makeSnapshotForUI(
            resolution, requestId, [this, streamId, uuid, sender, requestId]( const Json& message ) {
                sendAsyncResponse( sender, message, uuid );
                Log::info( cc::info( "Stream " ) + cc::note( streamId ) + cc::info( " is snapshoted for UI" ) +
                           ", requestId: " + cc::info( requestId ) );
            } );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Make snapshot for UI exception: " ) ) + cc::attention( ex.what() ) +
                    "requestId: " + requestId );
        sendAsyncResponse( sender, createError( ex.what() ), uuid );
        std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
        if( auto status = findStreamStatus( streamId ) ) {
            status->lastSnapshotCreatingForUiError = ex.what();
        }
    }
}

void CamerasAddon::makeStreamSnapshotForCloud( const JsonObject& params ) {
    m_snapshotCache->appendRequest( params );
}

void CamerasAddon::makeStreamSnapshotForCloudInternal( const JsonObject& params ) {
    Log::debug( cc::debug( "Make stream snapshot for Cloud: " ) + cc::j( params ) );
    std::string streamId;
    try {
        const std::string streamId = tryGetNonEmptyString( params, "streamId" );
        const std::string snapshotId = tryGetNonEmptyString( params, "snapshotId" );
        const std::string cacheId = tryGetNonEmptyString( params, "cacheId" );

        Json metaData;
        json_get( params, "metaData", metaData );
        if( !json_contains( metaData, "device_id" ) ) {
            LOGGER_INFO_RAW( cc::info( "device id not found in meta data, added by firmware for streamId: " ) +
                             cc::note( streamId ) );
            auto deviceId = getDeviceId( streamId );
            metaData[ "device_id" ] = deviceId;
        }

        auto lock = tryCreateUniqueLock( m_mutex );
        auto it = m_cameras.find( streamId );
        if( it == m_cameras.end() ) {
            throw StreamSnapshotException( "Stream is not created: " + streamId );
        }
        auto& camera = *it->second;
        if( !camera.isRunning() ) {
            throw StreamSnapshotException( "Stream is not running: " + streamId );
        }

        using namespace std::placeholders;

        removeCachedImageIfExpired( streamId );
        if( m_snapshotCache->isCachedImage( streamId ) ) {
            auto imagePath = m_snapshotCache->getCachedImagePath( streamId );
            auto metadata = m_snapshotCache->getCachedImageMetadata( streamId );
            FileRemoverSharedPtr remover = m_imageRemovers[ imagePath ];
            ResultCallback resultCallback;
            saveRecodingToCloud(
                streamId,
                imagePath,
                [this, streamId, cacheId, snapshotId]( const Json& ) {
                    auto imageInfo = m_snapshotCache->getCachedImageInfo( streamId );
                    m_snapshotCache->setRequestStatusDone( cacheId );
                    emitEventToPlugin( streamId, StreamSnapshotted( streamId, snapshotId, imageInfo[ "storageKey" ] ) );
                },
                remover,
                metadata );
        }
        else {
            const bool useImageCache = true;
            camera.makeSnapshotForCloud(
                metaData,
                std::bind( &CamerasAddon::saveFileToCloud, this, _1, _2, _3, std::string(), _4, useImageCache ),
                snapshotId,
                [this, metaData, streamId, snapshotId, cacheId]( const Json& storageInfo ) {
                    emitEventToPlugin( streamId,
                                       StreamSnapshotted( streamId, snapshotId, storageInfo[ "storageKey" ] ) );
                    Log::info( cc::info( "Stream " ) + cc::note( streamId ) + cc::info( " is snapshoted for Cloud: " ) +
                               cc::note( snapshotId ) );
                    m_snapshotCache->addSnapshotToCache( streamId, storageInfo, metaData );
                    m_snapshotCache->setRequestStatusDone( cacheId );
                },
                [this, streamId, snapshotId]( const std::string& errorMessage ) {
                    emitEventToPlugin( streamId, StreamSnapshotError( streamId, snapshotId, errorMessage ) );
                    std::unique_lock< std::mutex > statusLock( m_streamStatusMutex );
                    if( auto status = findStreamStatus( streamId ) ) {
                        status->lastSnapshotCreatingForCloudError = errorMessage;
                    }
                    statusLock.unlock();
                    Log::error( cc::error( "Stream " ) + cc::note( streamId ) + cc::error( " snapshoting error: " ) +
                                cc::attention( errorMessage ) );
                } );
        }
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Make snapshot for Cloud exception: " ) ) + cc::attention( ex.what() ) );
        emitEventToPlugin( streamId, StreamSnapshotError( ex.what() ) );
        std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
        if( auto status = findStreamStatus( streamId ) ) {
            status->lastSnapshotCreatingForCloudError = ex.what();
        }
    }
}

void CamerasAddon::removeCachedImageIfExpired( const std::string& streamId ) {
    if( m_snapshotCache->isCachedImage( streamId ) ) {
        if( m_snapshotCache->isExpiredCachedImage( streamId ) ) {
            auto imagePath = m_snapshotCache->getCachedImagePath( streamId );
            m_snapshotCache->removeCachedInfo( streamId );
            m_imageRemovers.erase( imagePath );
        }
    }
}

Camera& CamerasAddon::findOrCreateCamera( const std::string& streamId ) {
    auto lock = tryCreateUniqueLock( m_mutex );
    auto it = m_cameras.find( streamId );
    if( it != m_cameras.end() ) {
        return *it->second;
    }

    const JsonObject existingStream =
        getDBClient().findOne( CameraStreamsDBName, JSON( "streamId" << streamId ), nullptr );
    if( existingStream.empty() ) {
        throw CamerasAddonException( "Stream with id \"" + streamId + "\" does not exist" );
    }
    auto rtspUrl = tryGetNonEmptyString( existingStream, "rtspUrl" );
    auto camera = createCamera( streamId, rtspUrl );
    auto camerasIt = m_cameras.emplace( streamId, std::move( camera ) );

    PluginId pluginId = json_get_str( existingStream, "pluginId" );
    if( !pluginId.empty() ) {
        std::lock_guard< std::mutex > lock( m_pluginIdsMutex );
        m_pluginIds.emplace( streamId, pluginId );
    }

    auto streamName = tryGetNonEmptyString( existingStream, "streamName" );
    auto description = json_get_str( existingStream, "description" );
    StreamStatus status(
        std::move( pluginId ), std::move( streamName ), std::move( description ), std::move( rtspUrl ), Clock::now() );

    std::unique_lock< std::mutex > statusLock( m_streamStatusMutex );
    m_streamStatuses.emplace( streamId, std::move( status ) );
    statusLock.unlock();

    return *camerasIt.first->second;
}

void CamerasAddon::updateIpAddress( const JsonObject& params ) {
    Log::debug( cc::debug( "Ip address is updated: " ) + cc::j( params ) );
    auto networkType = json_get_str( params, "type" );
    if( networkType.empty() ) {
        Log::error( cc::error( "Empty network type" ) );
        return;
    }
    if( networkType == "modem" ) {
        Log::warn( cc::warn( "Ignore modem network type" ) );
        return;
    }
    auto setIpAddress = [this]( const Json& params ) {
        Json ipv4;
        if( !json_get( params, "ipv4", ipv4 ) ) {
            return false;
        }
        std::string newIpAddress;
        if( ipv4.is_string() ) {
            newIpAddress = json_get_str( ipv4 );
        }
        else if( ipv4.is_object() ) {
            newIpAddress = json_get_str( ipv4, "ip" );
        }
        if( newIpAddress.empty() ) {
            return false;
        }
        m_currentIpAddress = newIpAddress;
        return true;
    };

    auto lock = tryCreateUniqueLock( m_mutex );
    if( !setIpAddress( params ) ) {
        Log::error( cc::error( "No network ip address: " ) + cc::j( params ) );
        return;
    }
    for( const auto& cameraPair : m_cameras ) {
        auto& camera = *cameraPair.second;
        auto oldLink = camera.getLiveRtspStreamLink();
        auto newLink = updateLiveRtspStreamLink( oldLink );
        if( newLink.empty() || newLink == oldLink ) {
            continue;
        }
        camera.setLiveRtspStreamLink( newLink );
        auto& streamId = cameraPair.first;
        emitEventToPlugin( streamId, LiveRtspStreamUpdated( streamId, newLink ) );
        Log::info( cc::info( "Stream link is updated: " ) + cc::note( newLink ) );
    }
}

void CamerasAddon::handleExternalEvent( const JsonObject& params ) {
    auto name = json_get_str( params, "event" );
    emit( CameraBaseEvent( name, Json( params ) ) );
}

void CamerasAddon::handleEventFromBus( const Services::Message& message ) {
    switch( message.msgSubClass ) {
    case NETWORK_BROADCAST_IP_ADDRESS_CHANGED:
        updateIpAddress( message.data );
        break;
    case NETWORK_CAMERA_CLOUD_TUNNEL_STATUS_CHANGED:
        handleCameraCloudTunnelStatusChanged( message.data );
        break;
    case NETWORK_INTERFACE_STATUS_CHANGED:
        handleNetworkInterfaceStatusChanged( message.data );
        break;
    default:
        break;
    }
}

void CamerasAddon::handleCameraCloudTunnelStatusChanged( const JsonObject& params ) {
    LOGGER_DEBUG( cc::debug( "Camera cloud tunnel status changed: " ) + cc::j( params ) );
    const std::string streamId = json_get_str( params, "streamId" );
    if( !streamId.empty() ) {
        const std::string tunnelStatus = json_get_str( params, "status" );
        if( tunnelStatus == Network::SshTunnel::StatusConnected ) {
            emitEventToPlugin( streamId, CloudTunnelEstablished( streamId ) );
        }
        else if( tunnelStatus == Network::SshTunnel::StatusRejected ) {
            emitEventToPlugin( streamId, CloudTunnelRejected( streamId ) );
        }
    }
}

void CamerasAddon::handleNetworkInterfaceStatusChanged( const JsonObject& params ) {
    LOGGER_DEBUG( cc::debug( "Network interface status changed : " ) + cc::j( params ) );
    const std::string type = json_get_str( params, "type" );
    if( type != Network::Resources::EthernetType && type != Network::Resources::WifiType ) {
        return;
    }
    std::string interfaceId = json_get_str( params, "interfaceId" );
    if( interfaceId.empty() ) {
        LOGGER_ERROR( cc::error( "Empty interface id provided with the NetworkInterfaceStatusChanged event" ) );
        return;
    }
    bool isConnected = false;
    if( !json_get( params, "connected", isConnected ) ) {
        LOGGER_ERROR( cc::error( "Param 'connected' is absent in the NetworkInterfaceStatusChanged event" ) );
        return;
    }

    const bool hadConnectedInterfaceBefore = !m_connectedNetworkInterfaces.empty();

    if( isConnected )
        m_connectedNetworkInterfaces.insert( std::move( interfaceId ) );
    else
        m_connectedNetworkInterfaces.erase( interfaceId );

    const bool haveConnectedInterfaceAfter = !m_connectedNetworkInterfaces.empty();

    if( hadConnectedInterfaceBefore != haveConnectedInterfaceAfter ) {
        if( haveConnectedInterfaceAfter )
            emitEventToAllPlugins( LocalNetworkConnected() );
        else
            emitEventToAllPlugins( LocalNetworkDisconnected() );
    }
}

void CamerasAddon::printStreamStatuses() const {
    std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
    for( const auto& status : m_streamStatuses ) {
        LOGGER_INFO_RAW( cc::info( "Stream " ) + cc::note( status.first ) + cc::info( " status: " ) +
                         cc::note( "\n" + status.second.dump() ) );
    }
}

Json CamerasAddon::getStreamInfo( const JsonObject& params ) {
    JsonObject result;
    LOGGER_DEBUG_RAW( cc::debug( "Addon: Getting camera stream info: " ) + cc::j( params ) );

    std::lock_guard< std::mutex > statusLock( m_streamStatusMutex );
    auto streamId = json_get_str( params, "streamId" );
    auto it = m_streamStatuses.find( streamId );

    if( it != m_streamStatuses.cend() ) {
        result = it->second.toJson();
        result[ "streamId" ] = std::move( streamId );
        return result;
    }

    LOGGER_ERROR_RAW( cc::error( "Addon: Getting camera stream info error: stream id not found: " ) +
                      cc::note( streamId ) );
    result[ "streamId" ] = std::move( streamId );
    result[ "status" ] = "unknown";
    return result;
}

/** Uses cloud storage remote interface to store media files on cloud storage, v1.
 *
 * v1: no redirects:
 *      - create metadata of a file;
 *      - create upload file data;
 *      - call remoteInterface->uploadFile(data, callback1)
 *
 * */
void CamerasAddon::saveFileToCloud( const FilePath& filePath,
                                    const ResultCallback& resultCallback,
                                    const FileRemoverSharedPtr& remover,
                                    const std::string& uuid,
                                    const Json& metaData,
                                    bool useImageCache ) {
    Json meta = createMetaDataForCloudUploading( metaData );
    namespace CloudStorage = Common::CloudStorageInterface;
    LOGGER_DEBUG_RAW( cc::debug( "First upload try: " ) + cc::note( filePath ) );
    CloudStorage::UploadFileData data( filePath, uuid, std::move( meta ) );
    auto error = m_cloudStorageInterface->uploadFile(
        data,
        CloudStorage::StorageType::Multimedia,
        [this, data, remover, resultCallback, filePath, useImageCache](
            const Json& userData, const std::string& error, const CloudStorage::FileKey& fileKey ) {
            if( error.empty() ) {
                LOGGER_INFO_RAW( cc::info( "File uploaded: " ) + cc::note( data.file ) );
                if( useImageCache && m_imageRemovers.find( filePath ) == m_imageRemovers.end() ) {
                    m_imageRemovers[ filePath ] = remover;
                }

                resultCallback( JSON( "storageKey" << fileKey.toJson() << "filePath" << filePath ) );
            }
            else {
                LOGGER_WARN_RAW( cc::warn( "Upload error: " ) + cc::note( error ) );

                addTask( [this, data, remover, resultCallback, filePath, useImageCache]() {
                    LOGGER_DEBUG_RAW( cc::debug( "Second upload try: " ) + cc::note( data.file ) );
                    auto error = m_cloudStorageInterface->uploadFile(
                        data,
                        CloudStorage::StorageType::Multimedia,
                        [this, data, remover, resultCallback, filePath, useImageCache](
                            const Json& userData, const std::string& error, const CloudStorage::FileKey& fileKey ) {
                            if( error.empty() ) {
                                if( useImageCache && m_imageRemovers.find( filePath ) == m_imageRemovers.end() ) {
                                    m_imageRemovers[ filePath ] = remover;
                                }
                                LOGGER_DEBUG_RAW( cc::info( "File uploaded: " ) + cc::note( data.file ) );
                                resultCallback( JSON( "storageKey" << fileKey.toJson() << "filePath" << filePath ) );
                            }
                            else {
                                LOGGER_ERROR_RAW( cc::error( "Upload error: " ) + cc::note( error ) );
                                resultCallback( createError( error ) );
                            }
                        } );
                    if( error == Common::CloudStorageInterface::Errors::Ok ) {
                        LOGGER_INFO_RAW( cc::info( "File upload started successfully" ) );
                    }
                    else {
                        LOGGER_ERROR_RAW( cc::error( "Upload error: " ) +
                                          cc::note( Common::CloudStorageInterface::toString( error ) ) );
                    }
                } );
            }
        } );
    if( error == Common::CloudStorageInterface::Errors::Ok ) {
        LOGGER_INFO_RAW( cc::info( "File upload is started successfully" ) );
    }
    else {
        LOGGER_ERROR_RAW( cc::error( "Upload error: " ) +
                          cc::note( Common::CloudStorageInterface::toString( error ) ) );
    }
}

void CamerasAddon::saveRecodingToCloud( const StreamId& streamId,
                                        const FilePath& filePath,
                                        const ResultCallback& resultCallback,
                                        const FileRemoverSharedPtr& remover,
                                        const Json& metaData ) {
    auto updatedMetaData = metaData;
    if( !json_contains( updatedMetaData, "device_id" ) ) {
        LOGGER_INFO_RAW( cc::info( "device id not found in meta data, added by firmware for streamId: " ) +
                         cc::note( streamId ) );
        auto deviceId = getDeviceId( streamId );
        updatedMetaData[ "device_id" ] = deviceId;
    }
    auto itParticleData = m_videoParticlesData.find( streamId );
    if( itParticleData == m_videoParticlesData.end() ) {
        LOGGER_DEBUG( cc::debug( "Didn't find a video particle for stream: " ) + cc::note( streamId ) );
        saveFileToCloud( filePath, resultCallback, remover, std::string(), updatedMetaData, false );
        return;
    }

    VideoParticleData particleData = std::move( *itParticleData->second );
    m_videoParticlesData.erase( itParticleData );

    auto prepareUploadCallback =
        [this, filePath, resultCallback, remover, updatedMetaData, particleData]( const Json& response ) {
            // The cloud.<anything> report format is used: "NMA Route Cloud Calls Specs"
            //  {
            //      file_uuid: "1234-....",
            //      preview_uuid: "2345-..."
            //  }

            std::string fileUuid = json_get_str( response, Resources::UploadPreviewFields::FileUuid );
            std::string previewUuid = json_get_str( response, Resources::UploadPreviewFields::PreviewUuid );

            saveFileToCloud( filePath, resultCallback, remover, fileUuid, updatedMetaData, false );

            // no reason to upload a preview if it cannot be linked to the stream recording
            if( !fileUuid.empty() && !previewUuid.empty() ) {
                saveFileToCloud( particleData.m_filePath,
                                 particleData.m_resultCallback,
                                 particleData.m_remover,
                                 previewUuid,
                                 particleData.m_metaData,
                                 false );
            }
            else {
                Log::warn( cc::warn( "Prepare upload request failed: " ) + cc::j( response ) );
                particleData.m_resultCallback( createError( "upload preparing failed" ) );
            }
        };

    namespace CloudStorage = Common::CloudStorageInterface;
    const auto prepareUploadType = CloudStorage::PrepareUploadType::UploadAndPreview;
    CloudStorage::storagePrepareUpload( *this, prepareUploadType, prepareUploadCallback, std::chrono::seconds( 30 ) );
}

void CamerasAddon::saveParticleToCloud( const StreamId& streamId,
                                        const FilePath& filePath,
                                        const ResultCallback& resultCallback,
                                        const FileRemoverSharedPtr& remover,
                                        const Json& metaData ) {
    auto updatedMetaData = metaData;
    if( !json_contains( updatedMetaData, "device_id" ) ) {
        auto deviceId = getDeviceId( streamId );
        updatedMetaData[ "device_id" ] = deviceId;
    }
    LOGGER_DEBUG( cc::debug( "Storing particle data for stream: " ) + cc::note( streamId ) );
    auto particleData = std::make_unique< VideoParticleData >( filePath, resultCallback, remover, updatedMetaData );
    m_videoParticlesData[ streamId ] = std::move( particleData );
}

void CamerasAddon::applyNewLogLevel( int logLevel ) {
    switch( logLevel ) {
    case Log::PriorityLevel::TRACE:
        av_log_set_level( AV_LOG_TRACE );
        break;

    case Log::PriorityLevel::DEBUG:
        av_log_set_level( AV_LOG_DEBUG );
        break;

    case Log::PriorityLevel::INFO:
        av_log_set_level( AV_LOG_INFO );
        break;

    case Log::PriorityLevel::ERROR:
        av_log_set_level( AV_LOG_ERROR );
        break;

    case Log::PriorityLevel::WARN:
        av_log_set_level( AV_LOG_WARNING );
        break;

    case Log::PriorityLevel::FATAL:
        av_log_set_level( AV_LOG_FATAL );
        break;

    default:
        av_log_set_level( AV_LOG_INFO );
        break;
    }
}

Json CamerasAddon::createError( const std::string& errorMessage ) {
    return JSON( "error" << JSON( "message" << errorMessage ) );
}

Json CamerasAddon::createMetaDataForCloudUploading( const Json& userMetaData ) {
    Json metaData;
    if( !userMetaData.empty() ) {
        Json controllerInfo = userMetaData;
        controllerInfo[ "controller_uuid" ] = HubInfoHelper::getControllerUUID();
        metaData[ "controller_info" ] = std::move( controllerInfo );
    }
    return metaData;
}

void CamerasAddon::deleteStreamSavingToCloud( const std::string& streamId ) {
    Log::debug( cc::debug( "Delete stream saving to cloud: " ) + cc::note( streamId ) );
    try {
        auto lock = tryCreateUniqueLock( m_mutex );
        auto it = m_cameras.find( streamId );
        if( it == m_cameras.end() ) {
            return;
        }
        it->second->deleteStreamSavingToCloud();
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( std::string( "Stream deleting to cloud exception: " ) ) + cc::attention( ex.what() ) );
    }
}

void CamerasAddon::setConfigIntOption( ConfigReader& configReader,
                                       std::function< void( int ) > setter,
                                       const char* optionPath,
                                       int defaultValue ) {
    try {
        setter( configReader.getInt( optionPath ) );
    }
    catch( std::exception& ex ) {
        setter( defaultValue );
        Log::error( cc::error( "Unable found config option " ) + cc::note( optionPath ) + cc::error( ": " ) +
                    cc::attention( ex.what() ) );
        Log::error( cc::error( "Default value is set: " ) + cc::num( defaultValue ) );
    }
}

bool CamerasAddon::isParticleRecordingsLimitReached() const {
    constexpr int SimultaneousParticleRecordingsLimit = 2;  // needed in order to prevent loading hub's CPU on 100%
    int particleRecordingsCount = 0;
    auto lock = tryCreateUniqueLock( m_mutex );
    for( const auto& streamIdCameraPair : m_cameras ) {
        const CameraPtr& camera = streamIdCameraPair.second;
        if( camera->isRecordingStreamParticle() ) {
            ++particleRecordingsCount;
        }
    }
    return particleRecordingsCount >= SimultaneousParticleRecordingsLimit;
}

std::string CamerasAddon::updateLiveRtspStreamLink( std::string link ) {
    if( link.empty() ) {
        Log::error( cc::error( "Empty live RTSP Stream link: " ) );
        return "";
    }
    if( m_currentIpAddress.empty() ) {
        return link;
    }
    const std::string ipBeginMarker = "://";
    auto ipBegin = link.find( ipBeginMarker );
    if( ipBegin == link.npos ) {
        Log::error( cc::error( "Begin of ip address not found in stream link: " ) + cc::note( link ) );
        return link;
    }
    ipBegin += ipBeginMarker.size();
    const std::string ipEndMarker = ":";
    auto ipEnd = link.find( ipEndMarker, ipBegin );
    if( ipEnd == link.npos ) {
        Log::error( cc::error( "End of ip address not found in stream link: " ) + cc::note( link ) );
        return link;
    }
    link.replace( ipBegin, ipEnd - ipBegin, m_currentIpAddress );
    return link;
}

CamerasAddon::PluginId CamerasAddon::getPluginId( const StreamId& streamId ) const {
    std::lock_guard< std::mutex > lock( m_pluginIdsMutex );
    auto it = m_pluginIds.find( streamId );
    if( it != m_pluginIds.end() ) {
        return it->second;
    }
    Log::debug( cc::debug( "No plugin id for stream: " ) + cc::note( streamId ) );
    return "";
}

StreamParticleSettings CamerasAddon::findParticleSettings( const StreamId& streamId ) const {
    PluginId pluginId = getPluginId( streamId );
    if( !pluginId.empty() ) {
        auto it = m_particlesSettings.find( pluginId );
        if( it != m_particlesSettings.end() ) {
            return it->second;
        }
    }

    Log::info( cc::info( "Using default video particle settings for stream: " + cc::note( streamId ) ) );
    StreamParticleSettings defaultSettings;
    defaultSettings.resolutionDivisor = MinParticleResolutionDivisor;
    defaultSettings.preRecordDuration = DefaultParticlePreRecordTime;
    defaultSettings.recordDuration = DefaultParticleRecordTime;
    return defaultSettings;
}

StreamStatus* CamerasAddon::findStreamStatus( const CamerasAddon::StreamId& streamId ) {
    auto it = m_streamStatuses.find( streamId );
    if( it != m_streamStatuses.end() ) {
        return &it->second;
    }
    return nullptr;
}

void CamerasAddon::startLiveStreams() {
    auto lock = tryCreateUniqueLock( m_mutex );
    Json fieldsToReturn = JSON( "streamId" << 1 << "pluginId" << 1 << "rtspStreamStatus" << 1 );
    auto streams = getDBClient().findAll( CameraStreamsDBName, JSON_EMPTY_OBJ, &fieldsToReturn );
    Log::debug( cc::debug( "startLiveStreams : " ) + cc::j( streams ) );
    for( const auto& stream : streams ) {
        auto pluginId = json_get_str( stream, "pluginId" );
        auto rtspStreamStatus = json_get_str( stream, "rtspStreamStatus" );
        // NOTE: This condition is always false (bug), but the following stream initialization is also wrong.
        // Stream and it's statuses must be initialized via addon functions.
        if( pluginId.empty() && rtspStreamStatus == "running" ) {
            auto streamId = json_get_str( stream, "streamId" );
            auto& camera = findOrCreateCamera( streamId );
            camera.createLiveRtspStream();
        }
    }
}

void CamerasAddon::readConnectedNetworkInterfaces( const std::vector< Json >& connectedInterfaces ) {
    using namespace Network::Resources;
    for( const Json& interface : connectedInterfaces ) {
        const std::string type = json_get_str( interface, InterfaceInfo::Fields::Type );
        if( type == EthernetType || type == WifiType ) {
            std::string interfaceId = json_get_str( interface, InterfaceInfo::Fields::Id );
            if( !interfaceId.empty() ) {
                m_connectedNetworkInterfaces.insert( std::move( interfaceId ) );
            }
        }
    }
}

void CamerasAddon::onSubscriberAdded( const PluginId& pluginId ) {
    if( m_connectedNetworkInterfaces.empty() ) {
        emit( pluginId, LocalNetworkDisconnected() );
    }
}

void CamerasAddon::emitEventToPlugin( const StreamId& streamId, const CameraBaseEvent& event ) {
    auto pluginId = getPluginId( streamId );
    if( !pluginId.empty() ) {
        emit( pluginId, event );
    }
}

void CamerasAddon::emitEventToAllPlugins( const CameraBaseEvent& event ) {
    std::set< PluginId > uniquePluginIds;
    {
        std::lock_guard< std::mutex > lock( m_pluginIdsMutex );
        for( const auto& streamIdPluginIdPair : m_pluginIds ) {
            const PluginId& pluginId = streamIdPluginIdPair.second;
            uniquePluginIds.insert( pluginId );
        }
    }

    for( const PluginId& pluginId : uniquePluginIds ) {
        emit( pluginId, event );
    }
}

JsonObject CamerasAddon::getCameraStreams() {
    auto result = getDBClient().findAll( CameraStreamsDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto& cameraInfo : result ) {
        auto streamId = json_get_str( cameraInfo, "streamId" );
        if( !streamId.empty() ) {
            auto core = HubPlatform::CoreDRemoteInterface::makeInstance( *this );
            auto itemJson = core->getItem( streamId );
            auto cameraId = json_get_str( itemJson, "deviceId" );
            cameraInfo[ "cameraId" ] = cameraId;
        }
    }
    return result;
}

JsonObject CamerasAddon::getServiceInfo() {
    JsonObject info;
    auto streams = getCameraStreams();
    if( !streams.empty() ) {
        info[ "streams" ] = streams;
        return info;
    }
    return JSON_EMPTY_OBJ;
}

std::string CamerasAddon::getDeviceId( const std::string& streamId ) {
    auto record = getDBClient().findOne( CameraStreamsDBName, JSON( "streamId" << streamId ), nullptr );
    auto streamName = json_get_str( record, "streamName" );
    auto deviceRecord = getDBClient().findOne( HaDevicesDBName,
                                               JSON( "name" << streamName << "category"
                                                            << "camera" ),
                                               nullptr );
    return json_get_str( deviceRecord, "_id" );
}

CamerasAddon::VideoParticleData::VideoParticleData( FilePath filePath,
                                                    ResultCallback resultCallback,
                                                    FileRemoverSharedPtr remover,
                                                    Json metaData )
: m_filePath( std::move( filePath ) )
, m_resultCallback( std::move( resultCallback ) )
, m_remover( std::move( remover ) )
, m_metaData( std::move( metaData ) ) {}

}  // namespace Cameras
}  // namespace HubPlatform
