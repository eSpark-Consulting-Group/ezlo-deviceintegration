#include "vidoo4hub/vidoo_mediator/vidoo_mediator.h"

#include "vidoo4hub/core_manager/core_manager_interface.h"
#include "vidoo4hub/camera_manager/camera_manager_interface.h"
#include "vidoo4hub/stream_manager/stream_manager_interface.h"
#include "vidoo4hub/vidoo_manager/vidoo_manager_interface.h"
#include "vidoo4hub/camera_manager/camera_items_cache.h"
#include "vidoo4hub/vidoo_client/vidoo_client_interface.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "vidoo4hub/vidoo4hub_exception.h"

#include "cored/model/resources/cored_resources.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/exception_utils.h"

namespace HubPlatform {
namespace Vidoo4Hub {

namespace {
std::set< std::string > extractIds( const JsonArray& idArray ) {
    std::set< std::string > set;
    for( auto& id : idArray )
        set.emplace( id.get< std::string >() );
    return set;
}

}  // namespace

VidooMediator::VidooMediator( ICoreManager& coreManager,
                              IVidooClient& vidooClient,
                              ICameraManager& cameraManager,
                              IStreamManager& streamManager,
                              IVidooManager& vidooManager )
: m_coreManager( coreManager )
, m_vidooClient( vidooClient )
, m_cameraManager( cameraManager )
, m_streamManager( streamManager )
, m_vidooManager( vidooManager ) {}

void VidooMediator::handleItemUpdate( const JsonObject& message ) {
    using namespace CoreD::Model::Resources;
    LOGGER_DEBUG_RAW( cc::debug( "handleItemUpdate" ) );

    LOGGER_TRACE_RAW( cc::j( message ) );
    const auto itemId = json_get_str( message, ItemFields::IdFieldName );
    if( itemId.empty() ) {
        LOGGER_WARN_RAW( cc::error( "empty item id" ) );
        return;
    }

    JsonObject value;
    if( !json_get( message, ItemFields::ValueFieldName, value ) ) {
        LOGGER_ERROR_RAW( cc::error( "empty item value" ) );
        return;
    }

    auto& itemsCache = m_cameraManager.getCameraItemsCache();
    auto cacheData = itemsCache.findItem( itemId );

    if( !cacheData ) {
        LOGGER_TRACE_RAW( cc::debug( "Not camera item: " + itemId ) );
        return;
    }

    auto& itemType = cacheData->second.m_itemType;
    auto& cameraId = cacheData->second.m_cameraId;

    switch( itemType ) {
    case CameraItemsCache::ItemType::Snapshot:
        handleSnapshotItemUpdate( cameraId, itemId, value );
        break;
    case CameraItemsCache::ItemType::MakeRecording:
        handleMakeRecordingItemUpdate( cameraId, itemId, value );
        break;
    case CameraItemsCache::ItemType::StopRecording:
        handleStopRecordingItemUpdate( cameraId, itemId, value );
        break;
    default:
        LOGGER_TRACE_RAW(
            cc::debug( "Handler for item type not implemented: " + cc::num10( static_cast< int >( itemType ) ) ) );
    };
}

void VidooMediator::handleCamerasStartRtspStreaming( const JsonObject& message ) {
    using namespace Vidoo4Hub::Resources;
    LOGGER_TRACE_RAW( cc::j( message ) );
    std::string localRtspUrl = json_get_str( message, BusMessageFields::localRtspUrl );
    if( localRtspUrl.empty() ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to process RTSP stream started broadcast: empty localRtspUrl" ) );
        return;
    }

    auto streamId = json_get_str( message, BusMessageFields::streamId );
    if( streamId.empty() ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to process RTSP stream started broadcast: empty streamId" ) );
        return;
    }
    m_cameraManager.handleStreamStarted( streamId, localRtspUrl );
}

void VidooMediator::handleCamerasStopRtspStreaming( const JsonObject& message ) {
    using namespace Vidoo4Hub::Resources;
    LOGGER_TRACE_RAW( cc::j( message ) );
    auto streamId = json_get_str( message, BusMessageFields::streamId );
    if( streamId.empty() ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to process RTSP stream stopped broadcast: empty streamId" ) );
        return;
    }
    m_cameraManager.handleStreamStopped( streamId );
}

void VidooMediator::handleSnapshotItemUpdate( const std::string& cameraId,
                                              const std::string& itemId,
                                              const JsonObject& itemValue ) {
    using namespace Vidoo4Hub::Resources;

    if( !itemValue.is_number_integer() )
        throw Vidoo4HubError( "Snapshot item value is not integer" );
    int value = itemValue.get< int >();

    if( value == 0 )  // We don't reset default value
        return;

    LOGGER_DEBUG_RAW( cc::debug( "Resetting snapshot item: " + itemId ) );

    m_coreManager.core().updateItemValue( itemId, []( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, Json( 0 ) );
    } );

    auto method = prepareMethod( RPC::takeSnapshot );
    method[ "params" ] = JSON( MessageFields::cameraId << cameraId );
    m_vidooClient.sendMessage( method );
}

void VidooMediator::handleStopRecordingItemUpdate( const std::string& cameraId,
                                                   const std::string& itemId,
                                                   const JsonObject& itemValue ) {
    using namespace Vidoo4Hub::Resources;

    if( !itemValue.is_number_integer() )
        throw Vidoo4HubError( "StopRecording item value is not integer" );
    int value = itemValue.get< int >();

    if( value == 0 )  // We don't reset default value
        return;

    LOGGER_DEBUG_RAW( cc::debug( "Resetting StopRecording item: " + itemId ) );

    m_coreManager.core().updateItemValue( itemId, []( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, Json( 0 ) );
    } );

    auto method = prepareMethod( RPC::stopRecording );
    method[ "params" ] = JSON( MessageFields::cameraId << cameraId );
    m_vidooClient.sendMessage( method );
}

void VidooMediator::handleMakeRecordingItemUpdate( const std::string& cameraId,
                                                   const std::string& itemId,
                                                   const JsonObject& itemValue ) {
    using namespace Vidoo4Hub::Resources;
    using namespace Vidoo4Hub::Resources::DefaultValues;

    if( !itemValue.is_number_integer() )
        throw Vidoo4HubError( "MakeRecording item value is not integer" );
    int value = itemValue.get< int >();

    if( value == 0 )  // We don't reset default value
        return;
    else if( value < MinRecordingDuration ) {
        value = MinRecordingDuration;
        LOGGER_WARN_RAW( cc::debug( "Fix MakeRecording value to lower bound: " + cc::num10( value ) ) );
    }
    else if( value > MaxRecordingDuration ) {
        value = MaxRecordingDuration;
        LOGGER_WARN_RAW( cc::debug( "Fix MakeRecording value to upper bound: " + cc::num10( value ) ) );
    }

    LOGGER_DEBUG_RAW( cc::debug( "Resetting StopRecording item: " + itemId ) );

    m_coreManager.core().updateItemValue( itemId, []( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, Json( 0 ) );
    } );

    auto method = prepareMethod( RPC::makeRecording );
    method[ "params" ] = JSON( MessageFields::cameraId << cameraId << MessageFields::duration << value );
    m_vidooClient.sendMessage( method );
}

void VidooMediator::handleVidooMessage( const JsonObject& message ) {
    using namespace Vidoo4Hub::Resources;

    auto method = json_get_str( message, "method" );
    if( method.empty() )
        throw Vidoo4Hub::Vidoo4HubMessageError( "method is not set" );

    Json params;
    if( !json_get( message, "params", params ) )
        throw Vidoo4Hub::Vidoo4HubMessageError( "failed to parse params" );

    if( method == RPC::updateCameraList ) {
        handleUpdateCameraList( params );
        return;
    }
    if( method == RPC::updateObjectList ) {
        handleUpdateObjectList( params );
        return;
    }
    if( method == RPC::updatePeopleList ) {
        handleUpdatePeopleList( params );
        return;
    }
    if( method == RPC::updateQRCodeList ) {
        handleUpdateQRCodeList( params );
        return;
    }
    if( method == RPC::updateHotzoneList ) {
        handleUpdateHotzoneList( params );
        return;
    }
    else if( method == RPC::motion ) {
        handleMotion( params );
        return;
    }
    else if( method == RPC::hotzoneMotion ) {
        handleHotzoneMotion( params );
        return;
    }
    else if( method == RPC::requestCameraStreamsList ) {
        handleRequestCameraStreamsList();
        return;
    }
    else {
        throw Vidoo4Hub::Vidoo4HubMessageError( "unknown method: " + method );
    }
}

void VidooMediator::handleUpdateCameraList( const Json& params ) {
    LOGGER_DEBUG_RAW( cc::debug( "Handle update camera list request" ) );

    JsonArray cameraArray;
    if( !json_get_arr( params, "cameras", cameraArray ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to get cameras array" ) );
        return;
    }

    std::vector< Json > cameraList;
    cameraList.reserve( cameraArray.size() );
    for( auto& cameraJson : cameraArray )
        cameraList.emplace_back( std::move( cameraJson ) );

    m_cameraManager.updateCameraList( cameraList );
    m_coreManager.sync();
    m_vidooManager.syncUpdatedHotzones();
}

void VidooMediator::handleUpdateObjectList( const Json& params ) {
    LOGGER_DEBUG_RAW( cc::debug( "Handle update object list request" ) );

    JsonArray jsonArray;
    if( !json_get_arr( params, "objects", jsonArray ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to get known objects array" ) );
        return;
    }

    std::vector< Json > list;
    list.reserve( jsonArray.size() );
    for( auto& j : jsonArray )
        list.emplace_back( std::move( j ) );

    m_vidooManager.updateObjectList( list );
    m_vidooManager.syncUpdatedDetectableObjects();
}

void VidooMediator::handleUpdatePeopleList( const Json& params ) {
    LOGGER_DEBUG_RAW( cc::debug( "Handle update people list request" ) );

    JsonArray jsonArray;
    if( !json_get_arr( params, "people", jsonArray ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to get known people array" ) );
        return;
    }

    std::vector< Json > list;
    list.reserve( jsonArray.size() );
    for( auto& j : jsonArray )
        list.emplace_back( std::move( j ) );

    m_vidooManager.updatePeopleList( list );
    m_coreManager.sync();
    m_vidooManager.syncUpdatedDetectableObjects();
}

void VidooMediator::handleUpdateQRCodeList( const Json& params ) {
    LOGGER_DEBUG_RAW( cc::debug( "Handle update QR code list request" ) );

    JsonArray jsonArray;
    if( !json_get_arr( params, "qrcodes", jsonArray ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to get known qrcodes array" ) );
        return;
    }

    std::vector< Json > list;
    list.reserve( jsonArray.size() );
    for( auto& j : jsonArray )
        list.emplace_back( std::move( j ) );

    m_vidooManager.updateQRCodeList( list );
    m_coreManager.sync();
    m_vidooManager.syncUpdatedDetectableObjects();
}

void VidooMediator::handleUpdateHotzoneList( const Json& params ) {
    LOGGER_DEBUG_RAW( cc::debug( "Handle update hotzone list request" ) );

    JsonArray jsonArray;
    if( !json_get_arr( params, Vidoo4Hub::Resources::MessageFields::hotzonesList, jsonArray ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to get hotzones array" ) );
        return;
    }

    std::vector< Json > list;
    list.reserve( jsonArray.size() );
    for( auto& j : jsonArray )
        list.emplace_back( std::move( j ) );

    m_vidooManager.updateHotzoneList( list );
    m_vidooManager.syncUpdatedHotzones();
}

void VidooMediator::handleMotion( const Json& params ) {
    using namespace Vidoo4Hub::Resources;

    try {
        auto cameraId = json_get_str( params, DB::FieldNames::cameraId );
        if( cameraId.empty() ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to handle motion started: empty cameras id" ) );
            return;
        }

        const JsonArray& objects = params.at( MessageFields::objectIds );
        const JsonArray& persons = params.at( MessageFields::peopleIds );
        const JsonArray& qrCodes = params.at( MessageFields::qrCodeIds );

        IVidooManager::DetectedObjects detectedTypes;
        if( !objects.empty() )
            detectedTypes[ DetectableTypes::Type::Object ] = extractIds( objects );
        if( !persons.empty() )
            detectedTypes[ DetectableTypes::Type::Person ] = extractIds( persons );
        if( !qrCodes.empty() )
            detectedTypes[ DetectableTypes::Type::QRCode ] = extractIds( qrCodes );

        LOGGER_DEBUG_RAW( cc::debug( "Got 'motion' message for camera id: " + cameraId + " , objects: " ) +
                          cc::j( objects ) + cc::debug( " , persons: " ) + cc::j( persons ) +
                          cc::debug( " , qrcodes: " ) + cc::j( qrCodes ) );
        if( detectedTypes.empty() )
            m_cameraManager.setMotionEnded( cameraId );
        else
            m_cameraManager.setMotionStarted( cameraId );

        m_vidooManager.motion( cameraId, DefaultValues::defaultHotzoneId, detectedTypes );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle 'motion' method : " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
    }
}

void VidooMediator::handleHotzoneMotion( const Json& params ) {
    using namespace Vidoo4Hub::Resources;
    try {
        auto cameraId = json_get_str( params, DB::FieldNames::cameraId );
        if( cameraId.empty() ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to handle motion started: empty cameras id" ) );
            return;
        }

        LOGGER_DEBUG_RAW( cc::debug( "Got hotzone motion started for camera id: " + cameraId ) );

        auto hotzoneId = json_get_str( params, MessageFields::hotzoneId );

        const JsonArray& objects = params.at( MessageFields::objectIds );
        const JsonArray& persons = params.at( MessageFields::peopleIds );

        // NOTE: Since QR code is small it will be scanned closer to the camera
        // and it will intersect multiple hotzones.
        // Thus it is pointless to make it detectable for specific hotzone.

        IVidooManager::DetectedObjects detectedTypes;
        if( !objects.empty() )
            detectedTypes[ DetectableTypes::Type::Object ] = extractIds( objects );
        if( !persons.empty() )
            detectedTypes[ DetectableTypes::Type::Person ] = extractIds( persons );

        LOGGER_DEBUG_RAW( cc::debug( "Got 'hotzone_motion' message for camera id: " + cameraId + " , objects: " ) +
                          cc::j( objects ) + cc::debug( " , persons: " ) + cc::j( persons ) );
        m_vidooManager.motion( cameraId, hotzoneId, detectedTypes );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle 'hotzone_motion' method : " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
    }
}

void VidooMediator::handleRequestCameraStreamsList() {
    using namespace Vidoo4Hub::Resources;
    auto method = prepareMethod( RPC::updateCameraStreamsList );
    method[ "params" ] = JSON( MessageFields::streamsList << m_streamManager.getCameraStreams() );
    m_vidooClient.sendMessage( method );
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
