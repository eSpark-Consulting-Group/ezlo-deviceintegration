#include "vidoo4hub/vidoo4hub_addon.h"

#include "vidoo4hub/version/vidoo4hub_addon_version.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "vidoo4hub/vidoo_client/vidoo_client_factory.h"

#include "cored/remote_interface/cored_remote_interface.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "json_utils/json_utils.h"

#include "system_utils/environment.h"

namespace HubPlatform {

const char* const Vidoo4HubAddonName = "vidoo4hub";
const char* const Vidoo4HubGatewayName = "vidoo_gateway";
const char* const Vidoo4HubGatewayPluginId = "vidoo_gateway_plugin_id";

Vidoo4HubAddon::Vidoo4HubAddon()
: Addons::Addon( Vidoo4HubAddonName ) {}

CommonUtils::AddonVersion Vidoo4HubAddon::getVersion() const {
    return Addons::Vidoo4Hub::getVersion();
}

Vidoo4HubAddon::~Vidoo4HubAddon() = default;

void Vidoo4HubAddon::handleMethodCall( const JsonObject& request,
                                       MessageSubClasses requestType,
                                       const std::string& sender ) {
    switch( requestType ) {
    case VIDOO_SERVICE_MESSAGE: {
        addTask( [this, request]() { handleVidooMessage( request ); } );
        break;
    }
    }
}

void Vidoo4HubAddon::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case LOG_CMD:
        handleLogCommand( message, sender );
        break;
    case EVENT:
        handleEventMessage( message.msgSubClass, message.data );
        break;
    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Vidoo4HubAddon::handleEventMessage( const int subClass, const JsonObject& obj ) {
    try {
        switch( subClass ) {
        case HUB_ITEM_UPDATED:
            addTask( [obj, this]() { m_vidooMediator->handleItemUpdate( obj ); } );
            break;
        case CAMERAS_BROADCAST_START_RTSP_STREAMING: {
            addTask( [obj, this]() { m_vidooMediator->handleCamerasStartRtspStreaming( obj ); } );
            break;
        }
        case CAMERAS_BROADCAST_STOP_RTSP_STREAMING:
            addTask( [obj, this]() { m_vidooMediator->handleCamerasStopRtspStreaming( obj ); } );
            break;
        default:
            break;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Event handler error: " ) + cc::note( CommonUtils::whatError( ex ) ) );
    }
}

void Vidoo4HubAddon::setUp() {
    Addon::setUp();

    join( GroupsNames::BROADCAST_GROUP );
    join( GroupsNames::CAMERAS_BROADCASTS );
    join( GroupsNames::DEVICE_GROUP );  // capture hub.item.updated events

    m_core = std::make_unique< CoreDRemoteInterface::CoreDRemoteInterface >( *this );
    m_coreManager = std::make_unique< Vidoo4Hub::CoreManager >( *m_core );

    std::string gatewayId;
    while( true ) {
        gatewayId = ensureGateway();
        if( !gatewayId.empty() )
            break;
        Log::warn( cc::warn( "Failed to ensure gateway" ) );
        std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
    }

    m_vidooDevice = std::make_unique< Vidoo4Hub::VidooDevice >( *this, *m_coreManager, gatewayId );
    m_streamManager = std::make_unique< Vidoo4Hub::StreamManager >( *this );
    m_cameraManager = std::make_unique< Vidoo4Hub::CameraManager >(
        *this, *m_coreManager, *m_streamManager, m_vidooDevice->getDeviceId(), gatewayId );
    m_cameraManager->cleanItemValues();

    m_dictionaryManager = std::make_unique< Vidoo4Hub::DictionaryManager >( *m_coreManager );
    m_vidooMangerResources = std::make_unique< Vidoo4Hub::VidooManagerResources >( *m_vidooDevice, *m_cameraManager );
    m_vidooManager = std::make_unique< Vidoo4Hub::VidooManager >( *m_vidooMangerResources, *m_dictionaryManager );
    m_vidooClient =
        Vidoo4Hub::createVidooClient( *this, [this]( const JsonObject& msg ) { handleVidooMessage( msg ); } );
    m_vidooMediator = std::make_unique< Vidoo4Hub::VidooMediator >(
        *m_coreManager, *m_vidooClient, *m_cameraManager, *m_streamManager, *m_vidooManager );

    m_vidooClient->setUp();
}

void Vidoo4HubAddon::tearDown() {
    m_vidooClient->tearDown();
    Addon::tearDown();
}

JsonObject Vidoo4HubAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    JsonObject answer;
    try {
        switch( requestType ) {
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
            helper.backupDatabase( Vidoo4Hub::Resources::DB::TableNames::VidooCamerasDBName );
            helper.backupDatabase( Vidoo4Hub::Resources::DB::TableNames::VidooDeviceDBName );
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

int Vidoo4HubAddon::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    helper.restoreDatabase( Vidoo4Hub::Resources::DB::TableNames::VidooCamerasDBName );
    helper.restoreDatabase( Vidoo4Hub::Resources::DB::TableNames::VidooDeviceDBName );
    if( m_cameraManager )
        m_cameraManager->reloadCameras();
    return helper.getReturnCode();
}

void Vidoo4HubAddon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    Addon::handleLegacyMethodCall( json, sender );
}

void Vidoo4HubAddon::handleVidooMessage( const JsonObject& msg ) {
    using namespace Vidoo4Hub::Resources;

    try {
        m_vidooMediator->handleVidooMessage( msg );
    }
    catch( const Vidoo4Hub::Vidoo4HubMessageError& e ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to handle Vidoo message: " ) + cc::note( std::string( e.what() ) ) );
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to handle Vidoo message: exception: " ) +
                         cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to handle Vidoo message: unknown exception" ) );
    }
}

std::string Vidoo4HubAddon::ensureGateway() {
    using namespace HubPlatform::CoreD;
    if( !m_core ) {
        Log::error( cc::error( "Failed to ensureGateway: CoreD remote interface is nullptr" ) );
        return {};
    }

    std::string gatewayName = Vidoo4HubGatewayName;
    std::vector< std::pair< std::string, std::string > > unreachableReasons;
    std::vector< std::pair< std::string, std::string > > unreachableActions;

    Model::Gateway gateway( gatewayName, unreachableReasons, unreachableActions, Vidoo4HubGatewayPluginId );

    auto result = m_core->ensureGateway( gateway );
    if( result.second )
        return result.first;
    return {};
}

}  // namespace HubPlatform
