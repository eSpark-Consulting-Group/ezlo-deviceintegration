#include "cameras/bindings/cameras_client.h"
#include "cameras/common_res.h"

#include "addon_interfaces/addon.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {

CamerasClient::CamerasClient( Addons::Addon& hostAddon )
: ScriptClient( Addons::Addon::createQualifiedName( CamerasAddonName ), hostAddon ) {}

JsonObject CamerasClient::addCameraStream( const std::string& pluginId,
                                           const std::string& streamId,
                                           const std::string& streamName,
                                           const std::string& rtspUrl,
                                           const std::string& description ) {
    auto data = JSON( "pluginId" << pluginId << "streamId" << streamId << "streamName" << streamName << "rtspUrl"
                                 << rtspUrl << "description" << description );
    return sendRequest( CAMERAS_ADD_STREAM, std::move( data ) );
}

JsonObject CamerasClient::removeCameraStream( const std::string& streamId ) {
    return sendRequest( CAMERAS_REMOVE_STREAM, JSON( "streamId" << streamId ) );
}

void CamerasClient::createLiveRtspStream( const std::string& streamId ) {
    JsonObject params = JSON( "streamId" << streamId );
    sendMethodCall( MessageSubClasses::CAMERAS_CREATE_LIVE_RTSP_STREAM, std::move( params ) );
}

void CamerasClient::deleteLiveRtspStream( const std::string& streamId ) {
    JsonObject params = JSON( "streamId" << streamId );
    sendMethodCall( MessageSubClasses::CAMERAS_DELETE_LIVE_RTSP_STREAM, std::move( params ) );
}

void CamerasClient::createStreamSavingToCloud( const std::string& streamId,
                                               int segmentDurationSec,
                                               int secondsBeforeStart,
                                               int segmentsLimit,
                                               const std::string& format ) {
    JsonObject params =
        JSON( "streamId" << streamId << "segmentDurationSec" << segmentDurationSec << "secondsBeforeStart"
                         << secondsBeforeStart << "segmentsLimit" << segmentsLimit << "format" << format );
    sendMethodCall( MessageSubClasses::CAMERAS_CREATE_STREAM_SAVING_TO_CLOUD, std::move( params ) );
}

void CamerasClient::deleteStreamSavingToCloud( const std::string& streamId ) {
    JsonObject params = JSON( "streamId" << streamId );
    sendMethodCall( MessageSubClasses::CAMERAS_DELETE_STREAM_SAVING_TO_CLOUD, std::move( params ) );
}

void CamerasClient::startStreamSavingToCloud( const std::string& streamId,
                                              const Json& metaData,
                                              bool makeVideoParticle ) {
    JsonObject params = JSON( "streamId" << streamId << "makeVideoParticle" << makeVideoParticle );
    if( !metaData.empty() ) {
        params[ "metaData" ] = metaData;
    }
    sendMethodCall( MessageSubClasses::CAMERAS_START_STREAM_SAVING_TO_CLOUD, std::move( params ) );
}

void CamerasClient::stopStreamSavingToCloud( const std::string& streamId ) {
    JsonObject params = JSON( "streamId" << streamId );
    sendMethodCall( MessageSubClasses::CAMERAS_STOP_STREAM_SAVING_TO_CLOUD, std::move( params ) );
}

void CamerasClient::setVideoParticleSettings( const std::string& pluginId,
                                              int preRecordDurationSec,
                                              int recordDurationSec,
                                              int resolutionDivisor,
                                              const std::string& format ) {
    JsonObject params =
        JSON( "pluginId" << pluginId << "preRecordDurationSec" << preRecordDurationSec << "recordDurationSec"
                         << recordDurationSec << "resolutionDivisor" << resolutionDivisor << "format" << format );
    sendMethodCall( MessageSubClasses::CAMERAS_SET_VIDEO_PARTICLE_SETTINGS, std::move( params ) );
}

void CamerasClient::makeSnapshot( const std::string& streamId, const std::string& snapshotId, const Json& metaData ) {
    JsonObject params = JSON( "streamId" << streamId << "snapshotId" << snapshotId );
    if( !metaData.empty() ) {
        params[ "metaData" ] = metaData;
    }
    sendMethodCall( MessageSubClasses::CAMERAS_MAKE_SNAPSHOT_FOR_CLOUD, std::move( params ) );
}

void CamerasClient::createAudioStream( const Cameras::AudioStreamInfo& info ) {
    JsonObject params = info.toJson();
    getHost().sendMethodCall( GroupsNames::UI_GROUP, MessageSubClasses::CAMERAS_CREATE_AUDIO_STREAM, params );
}

void CamerasClient::deleteAudioStream( const std::string& streamId ) {
    JsonObject params = JSON( "streamId" << streamId );
    getHost().sendMethodCall( GroupsNames::UI_GROUP, MessageSubClasses::CAMERAS_DELETE_AUDIO_STREAM, params );
}

JsonObject CamerasClient::getAudioStreamInfo( const std::string& streamId ) {
    JsonObject params = JSON( "streamId" << streamId );
    return getHost().sendRequest( GroupsNames::UI_GROUP, MessageSubClasses::CAMERAS_GET_AUDIO_STREAM_INFO, params );
}

JsonObject CamerasClient::getStreamInfo( const std::string& streamId ) {
    return sendRequest( CAMERAS_GET_STREAM_INFO, JSON( "streamId" << streamId ) );
}

}  // namespace HubPlatform
