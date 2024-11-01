#include "zigbee/zigbee_addon.h"
#include "zigbee/zigbee_task_loop.h"
#include "zigbee/version/zigbee_addon_version.h"

#include "zigbee/script_events/node_added.h"
#include "zigbee/script_events/node_rejoined.h"
#include "zigbee/script_events/node_removed.h"
#include "zigbee/script_events/attribute_value_updated.h"
#include "zigbee/script_events/mfg_specific_attribute_value_updated.h"
#include "zigbee/script_events/unknown_attribute_value_updated.h"
#include "zigbee/script_events/include_device.h"
#include "zigbee/script_events/include_device_progress.h"
#include "zigbee/script_events/module_reset.h"
#include "zigbee/script_events/arm_command_received.h"
#include "zigbee/script_events/simple_command_received.h"
#include "zigbee/script_events/reachable_state_updated.h"
#include "zigbee/script_events/get_user_response_command_received.h"
#include "zigbee/script_events/door_lock_operation_event_notification_received.h"
#include "zigbee/script_events/door_lock_programming_event_notification_received.h"
#include "zigbee/script_events/user_codes_request_progress.h"
#include "zigbee/script_events/alert_received.h"
#include "zigbee/script_events/empty_alerts_list_response_received.h"
#include "zigbee/script_events/network_info_updated.h"
#include "zigbee/script_events/child_and_route_info_collected.h"

#include "json_utils/json_wrapper.h"
#include "service_interfaces/bus_wrapper.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

#include <cassert>
#include <string>
#include <system_utils/environment.h>
#include "common_utils/lock_helper.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/cpp14.h"
#include "system_utils/system.h"

#include "base_utils/zigbee/zigbee_environment.h"
#include "database_client/dbqueryparser.h"
#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/resources/enums/mode_status.h"
#include "zigbee/zigbee_log.h"
#include "zigbee/backup/backup_restore_status.h"
#include "zigbee/backup/zigbee_network_info.h"

#include "zigbee/model/enums/command_id.h"

#include "zigbee/interview_configurations/node_interview_configurations.h"
#include "zigbee/interview_configurations/node_interview_names_extractors.h"

#include "extensions_manager/remote_interface/em_remote_interface.h"

using namespace HubPlatform::Environment::Zigbee;

namespace HubPlatform {
namespace Zigbee {

constexpr const char InitStringCofigKey[] = "zigbee/initString";
constexpr const std::chrono::seconds ZigbeeStackInitTimeout{ 7 };
constexpr const std::chrono::minutes ZigbeeBackupRestoreTimeout{ 2 };
constexpr const int BackupRestoreInProgress = -1;

const char* const ZigbeeAddon::Name = "zigbee";

ZigbeeAddon::ZigbeeAddon()
: Addons::Addon( Name )
, m_isZigbeeInitialized( false )
, m_backupRestoreStatus( BackupRestoreInProgress ) {
    bootInit();
}

ZigbeeAddon::ZigbeeAddon( const std::string& name,
                          std::unique_ptr< Services::BusWrapper > impl,
                          std::unique_ptr< IDbClient > dbClient )
: Addons::Addon( name, std::move( impl ), std::move( dbClient ) )
, m_isZigbeeInitialized( false ) {
    bootInit();
}

CommonUtils::AddonVersion ZigbeeAddon::getVersion() const {
    return Addons::Zigbee::getVersion();
}

void ZigbeeAddon::setUp() {
    m_backupRestoreStatus = static_cast< int >( Backup::BackupRestoreStatus::Finished );
    setUpInternal( SetUpMode::Default );
}

void ZigbeeAddon::setUpInternal( SetUpMode mode ) {
    Addon::setUp();

    auto& db = getDBClient();

    m_configReader = std::make_unique< ConfigReader >();
    m_iZigbee = std::make_unique< ZigbeeInterface >( *this );
    m_nodeManager = std::make_unique< NodeManager >( getDBClient() );

    Services::Service::setReporter( std::make_unique< ZigbeeReporter >( getName(), m_nodeManager ) );

    std::vector< std::string > initParameters;
    try {
        ConfigReader::Path initParametersPath = ConfigReader::Path( *m_configReader, InitStringCofigKey );
        initParameters = m_configReader->getStringArray( initParametersPath );
    }
    catch( const std::exception& ex ) {
        ZB_LOG_ERR( "Failed to read config - will use defaults: exception = %s\n", ex.what() );
    }
    catch( ... ) {
        ZB_LOG_ERR( "Failed to read config - will use defaults: unknown exception occured\n" );
    }

    if( mode == SetUpMode::Default ) {
        const auto updateFirmwareCommand = Environment::getBinariesFolder() + "/zb-flash";
        system( updateFirmwareCommand.c_str() );
    }
    resetCycle();
    ZigbeeTaskLoop::init( [this, initParameters]() {
        IZigbeeResult result = IZigbeeResult::FAILED;
        IZigbeeResult loggedResult = IZigbeeResult::FAILED;
        while( ( isRunning() || m_backupRestoreStatus == BackupRestoreInProgress ) && !m_isZigbeeInitialized ) {
            if( result != IZigbeeResult::SUCCESS ) {
                result = m_iZigbee->init( initParameters );
            }

            if( IZigbeeResult::SUCCESS == result ) {
                m_iZigbee->mainLoop();
            }
            else {
                if( result != loggedResult ) {
                    loggedResult = result;
                    Log::debug( cc::debug( "Zigbee addon set up FAILED: " ) +
                                cc::debug( std::to_string( static_cast< int >( result ) ) ) );
                }
            }
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
        if( ( isRunning() || m_backupRestoreStatus == BackupRestoreInProgress ) && m_isZigbeeInitialized ) {
            m_iZigbee->mainLoop();
        }
    } );

    std::unique_lock< std::mutex > lock( m_mutex );
    if( !m_initCondVar.wait_for( lock, ZigbeeStackInitTimeout, [this]() -> bool { return m_isZigbeeInitialized; } ) ) {
        Log::debug( "Zigbee addon setup timeout !\n" );
    }
    else {
        Log::debug( "Zigbee addon setup done !\n" );
    }

    // Init task for reachable state monitoring
    if( mode == SetUpMode::Default ) {
        createPostponedTask( [this]( Services::PostponedTask& ) { initReachableStateRecheck(); } )
            ->start( std::chrono::minutes( 10 ), true );
    }
}

void ZigbeeAddon::tearDown() {
    ZigbeeTaskLoop::stopLoop();
    Addon::tearDown();
}

void ZigbeeAddon::restartAddon() {
    // TODO move to updatecmd
    std::string command = "kill " + std::to_string( getpid() );
    auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    if( result.second != 0 ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to restart zigbee addon: " ) + cc::note( result.first ) );
    }
}

void ZigbeeAddon::initReachableStateRecheck() {
    Log::debug( "Init reachable state recheck" );

    auto currentTime = std::chrono::system_clock::now();

    m_nodeManager->forEachNode( [&currentTime, this]( const ZigbeeNode& node ) {
        if( !node.isReachable() ) {
            return;
        }

        auto duration = currentTime - node.getLastUpdateTime();
        if( duration > node.getReachableStateRecheckTimeout() ) {
            // No need to try to send command to sleepy device
            if( node.isSleepy() ) {
                LOGGER_INFO_PRINT( cc::info( "Sleepy node is marked as unreachable, nodeId: 0x%04x" ).c_str(),
                                   node.getId() );
                updateDeviceReachable( node.getId(), false );
                return;
            }

            auto endpoint = node.findEndpoint( 1 );
            if( !endpoint ) {
                return;
            }
            auto basicCluster = endpoint->findCluster( static_cast< uint16_t >( ClusterId::Basic ) );
            if( !basicCluster ) {
                return;
            }
            auto attribute =
                basicCluster->findAttribute( static_cast< uint16_t >( BasicClusterAttributeId::ModelIdentifier ) );
            if( !attribute ) {
                return;
            }

            m_iZigbee->requestAttributeValue( node.getId(),
                                              endpoint->getId(),
                                              basicCluster->getId(),
                                              attribute->getId(),
                                              attribute->isMfgSpecific(),
                                              attribute->getMfgCode() );
        }
    } );
}

void ZigbeeAddon::handleStartInclude( const JsonObject& params ) {
    startInclude( params );
}

void ZigbeeAddon::handleStopInclude( const JsonObject& params ) {
    stopInclude();
}

void ZigbeeAddon::handleRemoveNode( const JsonObject& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    removeNode( nodeId );
}

void ZigbeeAddon::handleReset( const Json& params ) {
    moduleResetStatusChanged( ModeStatus::Invoked );
    reset();
}

void ZigbeeAddon::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    Service::handleMessageFromBus( message, sender );
}

JsonObject ZigbeeAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
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
    default:
        return Service::handleRequestFromBus( request, requestType );
    }

    return JSON_EMPTY_OBJ;
}

int ZigbeeAddon::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& tableName : Resources::DB::BackupTableNames ) {
        if( !helper.restoreDatabase( tableName ) ) {
            return helper.getReturnCode();
        }
    }

    setUpInternal( SetUpMode::BackupRestore );

    Json networkInfoJson;
    if( !helper.restoreJson( "zigbee/network_info", networkInfoJson ) )
        return helper.getReturnCode();

    m_iZigbee->restoreBackup( Backup::networkInfoFromJson( networkInfoJson ) );

    std::unique_lock< std::mutex > lock( m_backupRestoreMutex );
    if( !m_backupRestoreCondVar.wait_for( lock, ZigbeeBackupRestoreTimeout, [this]() -> bool {
            return m_backupRestoreStatus != BackupRestoreInProgress;
        } ) ) {
        Log::error( cc::error( "Zigbee backup restore timeout !\n" ) );
        m_backupRestoreStatus = static_cast< int >( Backup::BackupRestoreStatus::Timeout );
    }

    ZigbeeTaskLoop::stopLoop();
    helper.setReturnCode( m_backupRestoreStatus );
    return helper.getReturnCode();
}

bool ZigbeeAddon::handleAsyncRequestFromBus( const JsonObject& request,
                                             MessageSubClasses requestType,
                                             const std::string& uuid,
                                             const std::string& sender ) {
    switch( requestType ) {
    case BACKUP_CREATE:
        m_backupRequestInfo = std::make_unique< AsyncRequestInfo >( request, uuid, sender );
        m_iZigbee->collectNetworkInfo();
        break;
    case ZIGBEE_GET_INFO: {
        m_networkRequestInfo = std::make_unique< AsyncRequestInfo >( request, uuid, sender );
        m_iZigbee->getNetworkInfo();
        break;
    }

    default:
        return Service::handleAsyncRequestFromBus( request, requestType, uuid, sender );
    }
    return true;
}

// Zigbee public interface -----------------------------------------------------

void ZigbeeAddon::startInclude( const Json& params ) {
    m_interviewConfigurationsFilePatsh = json_get_str( params, Resources::RPCParams::InterviewConfigurationsFile );

    CommonUtils::LockHelper::global().tryStartTransaction( ZIGBEE_START_INCLUDE );

    Log::debug( "Try to start include" );
    IZigbeeResult result = m_iZigbee->startInclude();
}

void ZigbeeAddon::stopInclude() {
    Log::debug( "Try to stop include" );
    IZigbeeResult result = m_iZigbee->stopInclude();
}

void ZigbeeAddon::removeNode( const uint16_t nodeId ) {
    CommonUtils::LockHelper::global().tryStartTransaction( ZIGBEE_REMOVE_NODE );

    Log::debug( "removeNode for node: " + std::to_string( nodeId ) );
    m_iZigbee->removeNode( nodeId );
}

void ZigbeeAddon::reset() {
    CommonUtils::LockHelper::global().tryStartTransaction( ZIGBEE_RESET );

    Log::debug( "reset" );
    m_iZigbee->resetToDefault();
    moduleResetStatusChanged( ModeStatus::Started );
}

void ZigbeeAddon::setAttributeValue( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto clusterName = json_get_str( params, Resources::RPCParams::ClusterName );
    auto clusterId = clusterIdFromString( clusterName );
    auto attributeName = json_get_str( params, Resources::RPCParams::AttributeName );
    auto attributeId = attributeIdFromString( clusterId, attributeName );
    Json value = params[ Resources::RPCParams::Value ];

    auto zigbeeNode = m_nodeManager->findNode( nodeId );

    if( !zigbeeNode ) {
        Log::error( "Try to set attribute of unknown node: " + std::to_string( nodeId ) );
        return;
    }

    auto endpoint = zigbeeNode->findEndpoint( endpointId );
    if( !endpoint ) {
        Log::error( "Try to set attribute of unknown endpoint: " + std::to_string( endpointId ) );
        return;
    }

    auto cluster = endpoint->findCluster( static_cast< uint16_t >( clusterId ) );
    if( !cluster ) {
        Log::error( "Try to set attribute of unknown cluster: " + clusterName );
        return;
    }

    auto attribute = cluster->findAttribute( attributeId );
    if( !attribute ) {
        Log::error( "Try to set unknown attribute value: " + attributeName );
        return;
    }

    m_iZigbee->setAttributeValue(
        nodeId, endpointId, static_cast< uint16_t >( clusterId ), attributeId, attribute->getType(), value );
}

void ZigbeeAddon::setNetworkRadioChannel( const Json& params ) {
    auto value = json_get_int( params, Resources::RPCParams::Value );
    m_iZigbee->setNetworkRadioChannel( value );
}

void ZigbeeAddon::getChildAndRouteInfo( const Json& /*params*/ ) {
    m_iZigbee->getChildAndRouteInfo();
}

void ZigbeeAddon::sendCommand( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto clusterName = json_get_str( params, Resources::RPCParams::ClusterName );
    auto clusterId = static_cast< uint16_t >( clusterIdFromString( clusterName ) );
    auto commandName = json_get_str( params, Resources::RPCParams::CommandName );
    Json commandParams = params[ Resources::RPCParams::CommandParams ];

    auto zigbeeNode = m_nodeManager->findNode( nodeId );

    if( !zigbeeNode ) {
        Log::debug( "Try to send command to unknown node: " + std::to_string( nodeId ) );
        return;
    }

    if( !zigbeeNode->findEndpoint( endpointId ) ) {
        Log::debug( "Try to send command to unknown endpoint: " + std::to_string( endpointId ) );
        return;
    }

    if( !zigbeeNode->findEndpoint( endpointId )->findCluster( clusterId ) ) {
        Log::debug( "Try to send command to unknown cluster: " + clusterName );
        return;
    }

    m_iZigbee->sendCommand( nodeId, endpointId, clusterId, commandName, commandParams );
}

void ZigbeeAddon::requestAttributeValue( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto clusterName = json_get_str( params, Resources::RPCParams::ClusterName );
    auto clusterId = clusterIdFromString( clusterName );
    auto attributeName = json_get_str( params, Resources::RPCParams::AttributeName );
    auto attributeId = attributeIdFromString( clusterId, attributeName );

    requestAttributeValueInternal( nodeId, endpointId, static_cast< uint16_t >( clusterId ), attributeId, false );
}

void ZigbeeAddon::requestUserCodes( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto startUserId = json_get_int( params, Resources::RPCParams::StartUserId );
    auto endUserId = json_get_int( params, Resources::RPCParams::EndUserId );

    auto zigbeeNode = m_nodeManager->findNode( nodeId );

    if( !zigbeeNode ) {
        Log::debug( "Try to request user codes of unknown node: " + std::to_string( nodeId ) );
        return;
    }

    if( !zigbeeNode->findEndpoint( endpointId ) ) {
        Log::debug( "Try to request user codes of unknown endpoint: " + std::to_string( endpointId ) );
        return;
    }

    if( !zigbeeNode->findEndpoint( endpointId )->findCluster( ZCL_DOOR_LOCK_CLUSTER_ID ) ) {
        Log::debug( std::string( "Try to request user codes of unknown cluster: " ) + toString( ClusterId::DoorLock ) );
        return;
    }

    m_iZigbee->requestUserCodes( nodeId, endpointId, startUserId, endUserId );
}

void ZigbeeAddon::requestMfgSpecificAttributeValue( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto clusterId = json_get_int( params, Resources::RPCParams::ClusterId );
    auto attributeId = json_get_int( params, Resources::RPCParams::AttributeId );

    requestAttributeValueInternal( nodeId, endpointId, clusterId, attributeId, true );
}

void ZigbeeAddon::configureAttributeReport( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto clusterName = json_get_str( params, Resources::RPCParams::ClusterName );
    auto clusterId = clusterIdFromString( clusterName );
    auto attributeName = json_get_str( params, Resources::RPCParams::AttributeName );
    auto attributeId = attributeIdFromString( clusterId, attributeName );

    configureAttributeReportInternal(
        nodeId, endpointId, static_cast< uint16_t >( clusterId ), attributeId, params, false );
}

void ZigbeeAddon::configureMfgSpecificAttributeReport( const Json& params ) {
    auto nodeId = json_get_int( params, Resources::RPCParams::NodeId );
    auto endpointId = json_get_int( params, Resources::RPCParams::EndpointId );
    auto clusterId = json_get_int( params, Resources::RPCParams::ClusterId );
    auto attributeId = json_get_int( params, Resources::RPCParams::AttributeId );

    configureAttributeReportInternal( nodeId, endpointId, clusterId, attributeId, params, true );
}

void ZigbeeAddon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    Addon::handleLegacyMethodCall( json, sender );
}

void ZigbeeAddon::handleMethodCall( const JsonObject& json,
                                    MessageSubClasses requestType,
                                    const std::string& /*sender*/ ) {
    using MethodHandler = void ( ZigbeeAddon::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;

    static const MethodHandlersMap methodHandlers = {
        { ZIGBEE_START_INCLUDE, &ZigbeeAddon::handleStartInclude },
        { ZIGBEE_STOP_INCLUDE, &ZigbeeAddon::handleStopInclude },
        { ZIGBEE_REMOVE_NODE, &ZigbeeAddon::handleRemoveNode },
        { ZIGBEE_SET_ATTRIBUTE_VALUE, &ZigbeeAddon::setAttributeValue },
        { ZIGBEE_SEND_COMMAND, &ZigbeeAddon::sendCommand },
        { ZIGBEE_REQUEST_ATTRIBUTE_VALUE, &ZigbeeAddon::requestAttributeValue },
        { ZIGBEE_REQUEST_USER_CODES, &ZigbeeAddon::requestUserCodes },
        { ZIGBEE_REQUEST_MFG_SPECIFIC_ATTRIBUTE_VALUE, &ZigbeeAddon::requestMfgSpecificAttributeValue },
        { ZIGBEE_CONFIGURE_ATTRIBUTE_REPORT, &ZigbeeAddon::configureAttributeReport },
        { ZIGBEE_CONFIGURE_MFG_SPECIFIC_ATTRIBUTE_REPORT, &ZigbeeAddon::configureMfgSpecificAttributeReport },
        { ZIGBEE_RESET, &ZigbeeAddon::handleReset },
        { ZIGBEE_SET_NETWORK_RADIO_CHANNEL, &ZigbeeAddon::setNetworkRadioChannel },
        { ZIGBEE_GET_CHILD_AND_ROUTE_INFO, &ZigbeeAddon::getChildAndRouteInfo }
    };

    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        CommonUtils::LockHelper::global().postponeIfLocked( [=]() { ( this->*handler )( json ); } );
    }
}

void ZigbeeAddon::moduleResetStatusChanged( ModeStatus status ) {
    addTask( [this, status]() { emit( ModuleReset( status ) ); } );
}

const ZigbeeNode* ZigbeeAddon::findNode( uint16_t nodeId ) {
    return m_nodeManager->findNode( nodeId );
}

void ZigbeeAddon::stackInitialized() {
    ZB_LOG_DEBUG( "ZigbeeAddon::stackInitialized()\n" );
}

void ZigbeeAddon::nodeAdded( std::shared_ptr< ZigbeeNode > zigbeeNode ) {
    addTask( [this, zigbeeNode]() {
        Log::debug( "Node added with id: " + std::to_string( zigbeeNode->getId() ) );
        Log::debug( "New node added: " + cc::j( zigbeeNode->toJson() ) );
        m_nodeManager->add( zigbeeNode );

        emit( NodeAdded( zigbeeNode->getId() ) );
    } );
}

void ZigbeeAddon::nodeRemoved( const uint16_t nodeId ) {
    addTask( [this, nodeId]() {
        Log::info( cc::info( "Node removed with id: " + cc::num0x( nodeId ) + " ( " + cc::num10( nodeId ) + " )" ) );

        if( auto node = m_nodeManager->findNode( nodeId ) ) {
            auto manufacturerId = node->getManufacturerId();
            auto modelId = node->getModelId();

            m_nodeManager->remove( nodeId );
            emit( NodeRemoved( nodeId, manufacturerId, modelId ) );
        }

        CommonUtils::LockHelper::global().onTransactionStopped( ZIGBEE_REMOVE_NODE );
    } );
}

void ZigbeeAddon::nodeRejoined( uint16_t nodeId ) {
    addTask( [this, nodeId]() { emit( NodeRejoined( nodeId ) ); } );
}

void ZigbeeAddon::includeStatusChange( ModeStatus status ) {
    addTask( [this, status]() {
        emit( IncludeDevice( status ) );
        switch( status ) {
        case ModeStatus::Finished:
        case ModeStatus::Failed:
        case ModeStatus::Timeout:
            CommonUtils::LockHelper::global().onTransactionStopped( ZIGBEE_START_INCLUDE );
        }
    } );
}

void ZigbeeAddon::attributeValueUpdated( uint16_t nodeId,
                                         uint16_t endpointId,
                                         uint16_t clusterId,
                                         uint16_t attributeId,
                                         const std::string value ) {
    ZB_LOG_DEBUG( "ZigbeeAddon::attributeValueUpdated: 0x%04x 0x%04x 0x%04x 0x%04x %s\n",
                  nodeId,
                  endpointId,
                  clusterId,
                  attributeId,
                  value.c_str() );
    addTask( [this, nodeId, endpointId, clusterId, attributeId, value]() {
        auto zigbeeNode = m_nodeManager->findNode( nodeId );

        if( !zigbeeNode ) {
            ZB_LOG_DEBUG( "%s: invalid node 0x%04x\n", __PRETTY_FUNCTION__, nodeId );
            return;
        }

        ZigbeeClusterBase* cluster = nullptr;
        ZigbeeAttribute* attribute = nullptr;

        ZigbeeEndpoint* endpoint = zigbeeNode->findEndpoint( endpointId );
        if( !endpoint ) {
            ZB_LOG_DEBUG( "%s: invalid endpoint 0x%02x\n", __PRETTY_FUNCTION__, endpointId );
            return;
        }

        cluster = endpoint->findCluster( clusterId );
        if( !cluster ) {
            ZB_LOG_DEBUG(
                "%s: invalid cluster 0x%04x in endpoint 0x%02x\n", __PRETTY_FUNCTION__, clusterId, endpointId );
            return;
        }

        attribute = cluster->findAttribute( attributeId );
        if( !attribute ) {
            ZB_LOG_DEBUG( "Unknown attribute 0x%04x for cluster 0x%04x in endpoint 0x%02x\n",
                          attributeId,
                          clusterId,
                          endpointId );

            emit( UnknownAttributeValueUpdated( nodeId,
                                                endpointId,
                                                clusterId,
                                                cluster->getName(),
                                                attributeId,
                                                toString( static_cast< ClusterId >( clusterId ), attributeId ),
                                                value ) );
            return;
        }
        attribute->setValue( value );
        m_nodeManager->update( nodeId );

        if( attribute->isMfgSpecific() ) {
            emit( MfgSpecificAttributeValueUpdated( nodeId, endpointId, cluster->getId(), *attribute ) );
        }
        else {
            const auto& clusterName = cluster->getName();
            emit( AttributeValueUpdated( nodeId, endpointId, clusterName, *attribute ) );
        }
    } );
}

void ZigbeeAddon::armCommandReceived( uint16_t nodeId,
                                      uint16_t endpointId,
                                      IasAceArmCommandMode mode,
                                      const std::string pinCode ) {
    addTask( [this, nodeId, endpointId, mode, pinCode]() {
        emit( ArmCommandReceived( nodeId, endpointId, mode, pinCode ) );
    } );
}

void ZigbeeAddon::simpleCommandReceived( uint16_t nodeId,
                                         uint16_t endpointId,
                                         uint16_t clusterId,
                                         uint16_t commandId ) {
    addTask( [this, nodeId, endpointId, clusterId, commandId]() {
        auto zigbeeNode = m_nodeManager->findNode( nodeId );

        if( !zigbeeNode ) {
            ZB_LOG_DEBUG( "%s: invalid node 0x%04x\n", __PRETTY_FUNCTION__, nodeId );
            return;
        }

        ZigbeeClusterBase* cluster = nullptr;
        ZigbeeCommand* command = nullptr;

        ZigbeeEndpoint* endpoint = zigbeeNode->findEndpoint( endpointId );
        if( !endpoint ) {
            ZB_LOG_DEBUG( "%s: invalid endpoint 0x%02x\n", __PRETTY_FUNCTION__, endpointId );
            return;
        }

        cluster = endpoint->findCluster( clusterId );
        if( !cluster ) {
            ZB_LOG_DEBUG(
                "%s: invalid cluster 0x%04x in endpoint 0x%02x\n", __PRETTY_FUNCTION__, clusterId, endpointId );
            return;
        }

        std::string commandName = "";
        command = cluster->findCommand( commandId, CommandType::Generated );
        if( command ) {
            commandName = command->getName();
        }
        else {
            commandName = toString( static_cast< ClusterId >( cluster->getId() ),
                                    cluster->getDataHolder(),
                                    CommandType::Generated,
                                    commandId );
        }

        emit( SimpleCommandReceived( nodeId, endpointId, cluster->getName(), commandId, commandName ) );
    } );
}

void ZigbeeAddon::doorLockGetUserResponseReceived( uint16_t nodeId,
                                                   uint16_t endpointId,
                                                   uint16_t userId,
                                                   UserStatus userStatus,
                                                   UserType userType,
                                                   const std::string& pinCode ) {
    addTask( [this, nodeId, endpointId, userId, userStatus, userType, pinCode]() {
        emit( GetUserResponseReceived( nodeId, endpointId, userId, userStatus, userType, pinCode ) );
    } );
}

void ZigbeeAddon::doorLockOperatingEventNotificationReceived( uint16_t nodeId,
                                                              uint16_t endpointId,
                                                              DoorLockEventSource eventSource,
                                                              DoorLockOperationEvent event,
                                                              uint16_t userId,
                                                              const std::string& pinCode ) {
    addTask( [=]() {
        emit( DoorLockOperatingEventNotificationReceived( nodeId, endpointId, eventSource, event, userId, pinCode ) );
    } );
}

void ZigbeeAddon::doorLockProgrammingEventNotificationReceived( uint16_t nodeId,
                                                                uint16_t endpointId,
                                                                DoorLockEventSource eventSource,
                                                                DoorLockProgrammingEvent event,
                                                                uint16_t userId,
                                                                const std::string& pinCode,
                                                                UserType userType,
                                                                UserStatus userStatus ) {
    addTask( [=]() {
        emit( DoorLockProgrammingEventNotificationReceived(
            nodeId, endpointId, eventSource, event, userId, pinCode, userType, userStatus ) );
    } );
}

void ZigbeeAddon::alertsNotificationOrResponseReceived(
    uint16_t nodeId,
    uint8_t endpointId,
    const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
    HubPlatform::Zigbee::AlertSource type ) {
    if( alert_structures.empty() ) {
        addTask(
            [nodeId, endpointId, type, this]() { emit( EmptyAlertsListResponseReceived( nodeId, endpointId ) ); } );
    }
    else {
        addTask( [=]() {
            for( size_t i = 0; i < alert_structures.size(); ++i ) {
                emit(
                    AlertReceived( nodeId,
                                   endpointId,
                                   alert_structures.at( i ),
                                   type == HubPlatform::Zigbee::AlertSource::Notification ?
                                       Resources::Fields::Command::Names::ApplianceEventsAndAlerts::AlertsNotification :
                                       Resources::Fields::Command::Names::ApplianceEventsAndAlerts::GetAlertsResponse,
                                   type ) );
            }
        } );
    }
}

void ZigbeeAddon::updateDeviceReachable( uint16_t nodeId, bool isReachable ) {
    addTask( [this, nodeId, isReachable]() {
        auto zigbeeNode = m_nodeManager->findNode( nodeId );

        if( !zigbeeNode ) {
            ZB_LOG_DEBUG( "%s: invalid node 0x%04x\n", __PRETTY_FUNCTION__, nodeId );
            return;
        }

        bool previousReachableStatus = zigbeeNode->isReachable();
        bool isBroadcastedAfterReboot = m_nodeManager->isBroadcastedAfterReboot( nodeId );
        zigbeeNode->setIsReachable( isReachable );

        if( isReachable ) {
            zigbeeNode->deviceUpdated();
        }

        m_nodeManager->update( nodeId );

        if( !isBroadcastedAfterReboot ) {
            m_nodeManager->setBroadcastedAfterReboot( nodeId );
        }

        if( isReachable != previousReachableStatus || !isBroadcastedAfterReboot ) {
            emit( ReachableStateUpdated( nodeId, isReachable ) );
        }
    } );
}

void ZigbeeAddon::includeDeviceProgress( uint8_t progress ) {
    addTask( [this, progress]() { emit( IncludeDeviceProgress( progress ) ); } );
}

void ZigbeeAddon::userCodesRequestProgress( uint16_t nodeId, uint16_t endpointId, uint8_t progress ) {
    addTask(
        [this, nodeId, endpointId, progress]() { emit( UserCodesRequestProgress( nodeId, endpointId, progress ) ); } );
}

void ZigbeeAddon::networkDown() {
    ZB_LOG_DEBUG( "%s\n", __PRETTY_FUNCTION__ );
    // TODO check if stack data ( device database, various device tables are wiped correctly )
    ZB_LOG_DEBUG( "TODO check if stack data ( device database, various device tables are wiped correctly )\n" );

    m_nodeManager->reset();

    moduleResetStatusChanged( ModeStatus::Finished );
    m_iZigbee->startNewNetwork();
    CommonUtils::LockHelper::global().onTransactionStopped( ZIGBEE_RESET );
}

void ZigbeeAddon::networkUp() {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_isZigbeeInitialized ) {
        Log::debug( "Infinite loop during Zigbee stack initialization, restarting Zigbee addon" );
        restartAddon();
    }

    m_isZigbeeInitialized = true;
    lock.unlock();
    m_initCondVar.notify_all();
    Log::debug( "Zigbee stack initialized successfully\n" );

    addTask( [this]() { m_iZigbee->getNetworkInfo(); } );
}

void ZigbeeAddon::onNetworkChannelChanged() {
    addTask( [this]() { emit( NetworkInfoUpdated() ); } );
}

void ZigbeeAddon::onChildAndRouteInfo( uint16_t selfNodeId,
                                       std::vector< ChildData > children,
                                       std::vector< NeighborData > neighbors,
                                       std::vector< RouteData > routes ) {
    addTask( [=]() {
        emit( ChildAndRouteInfoCollected(
            selfNodeId, std::move( children ), std::move( neighbors ), std::move( routes ) ) );
    } );
}

void ZigbeeAddon::networkInfoCollected( const Backup::ZigbeeNetworkInfo& info, bool completed ) {
    addTask( [this, info, completed]() {
        bool sendNetworkInfoUpdated =
            completed && ( !m_networkInfoCache || *m_networkInfoCache != info ) && !m_networkRequestInfo;

        if( m_backupRequestInfo ) {
            CommonUtils::BackupHelper helper( getDBClient(), m_backupRequestInfo->m_request );

            if( !completed ) {
                sendAsyncResponse( m_backupRequestInfo->m_sender,
                                   helper.createFailedResponse( "Failed to read Zigbee chip information" ),
                                   m_backupRequestInfo->m_uuid );
                m_backupRequestInfo.reset();
                return;
            }

            static std::vector< std::string > BackupTables = {
                Resources::DB::TableNames::Devices,
                ZigbeeFlasherDBName,
            };

            for( auto& tableName : BackupTables ) {
                if( !helper.backupDatabase( tableName ) ) {
                    break;
                }
            }

            auto networkInfoJson = Backup::networkInfoToJson( info );
            Log::debug( "Zigbee network information: " + cc::j( networkInfoJson ) );
            helper.backupJson( "zigbee/network_info", networkInfoJson );

            sendAsyncResponse( m_backupRequestInfo->m_sender, helper.getResponse(), m_backupRequestInfo->m_uuid );
            m_backupRequestInfo.reset();
        }

        if( m_networkRequestInfo ) {
            if( !completed ) {
                auto response = JSON_EMPTY_OBJ;
                response[ "success" ] = false;
                setError( response, Errors::BACKUP_CREATE_FAILED, "Failed to read Zigbee chip information" );

                sendAsyncResponse( m_networkRequestInfo->m_sender, response, m_networkRequestInfo->m_uuid );
                m_networkRequestInfo.reset();
                return;
            }

            auto bytesToHex = []( const std::vector< uint8_t >& bytes ) {
                std::string result;
                result.reserve( bytes.size() * 2 );

                auto toSymbol = []( uint8_t byte ) {
                    return static_cast< char >( byte >= 0x0A ? 'A' + ( byte - 0x0A ) : '0' + byte );
                };

                for( auto byte : bytes ) {
                    result.push_back( toSymbol( byte >> 4 ) );
                    result.push_back( toSymbol( byte & 0x0F ) );
                }

                return result;
            };

            using namespace Resources::Fields;
            Json networkInfoJson;
            const auto& nwi = info;

            networkInfoJson[ NetworkInfo::PanId ] = nwi.panId;
            networkInfoJson[ NetworkInfo::RadioChannel ] = nwi.radioChannel;
            networkInfoJson[ NetworkInfo::EpanId ] =
                "0x" + bytesToHex( std::vector< uint8_t >( nwi.ePanId,
                                                           nwi.ePanId + sizeof nwi.ePanId / sizeof nwi.ePanId[ 0 ] ) );
            networkInfoJson[ NetworkInfo::Euid64 ] =
                "0x" +
                bytesToHex( std::vector< uint8_t >( nwi.eui64, nwi.eui64 + sizeof nwi.eui64 / sizeof nwi.eui64[ 0 ] ) );

            sendAsyncResponse( m_networkRequestInfo->m_sender, networkInfoJson, m_networkRequestInfo->m_uuid );
            m_networkRequestInfo.reset();
        }

        if( sendNetworkInfoUpdated ) {
            m_networkInfoCache = std::make_unique< Backup::ZigbeeNetworkInfo >( info );
            emit( NetworkInfoUpdated() );
        }
    } );
}

void ZigbeeAddon::restoreBackupCompleted() {
    std::unique_lock< std::mutex > lock( m_backupRestoreMutex );
    m_backupRestoreStatus = static_cast< int >( Backup::BackupRestoreStatus::Finished );
    lock.unlock();
    m_backupRestoreCondVar.notify_all();
}

void ZigbeeAddon::restoreBackupFailed( Backup::BackupRestoreStatus reason ) {
    std::unique_lock< std::mutex > lock( m_backupRestoreMutex );
    m_backupRestoreStatus = static_cast< int >( reason );
    lock.unlock();
    m_backupRestoreCondVar.notify_all();
}

std::unique_ptr< NodeInterviewConfigurations > ZigbeeAddon::provideNodeInterviewConfigurations(
    const std::string& manufacturerId,
    const std::string& modelId ) {
    try {
        if( m_interviewConfigurationsFilePatsh.empty() )
            return nullptr;

        ExtensionsManager::RemoteInterface extensionsInterface( *this );
        auto stream = extensionsInterface.getPluginFile( m_interviewConfigurationsFilePatsh );
        Json configurations = Json::object();
        *stream >> configurations;

        return NodeInterviewConfigurations::create( configurations, manufacturerId, modelId );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( e.what() );
        return nullptr;
    }
}

std::unique_ptr< NodeInterviewNamesExtractors > ZigbeeAddon::provideNodeInterviewNamesExtractors() {
    try {
        if( m_interviewConfigurationsFilePatsh.empty() )
            return nullptr;

        ExtensionsManager::RemoteInterface extensionsInterface( *this );
        auto stream = extensionsInterface.getPluginFile( m_interviewConfigurationsFilePatsh );
        Json configurations = Json::object();
        *stream >> configurations;

        return NodeInterviewNamesExtractors::create( configurations );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( e.what() );
        return nullptr;
    }
}

void ZigbeeAddon::requestAttributeValueInternal( uint16_t nodeId,
                                                 uint8_t endpointId,
                                                 uint16_t clusterId,
                                                 uint16_t attributeId,
                                                 bool isMfgSpecific ) {
    auto zigbeeNode = m_nodeManager->findNode( nodeId );

    if( !zigbeeNode ) {
        Log::error( "Try to request attribute of unknown node: " + std::to_string( nodeId ) );
        return;
    }

    auto endpoint = zigbeeNode->findEndpoint( endpointId );
    if( !endpoint ) {
        Log::error( "Try to request attribute of unknown endpoint: " + std::to_string( endpointId ) );
        return;
    }

    auto cluster = endpoint->findCluster( static_cast< uint16_t >( clusterId ) );
    if( !cluster ) {
        Log::error( "Try to request attribute of unknown cluster: " + std::to_string( clusterId ) );
        return;
    }

    auto attribute = cluster->findAttribute( attributeId );
    if( !attribute ) {
        Log::warn( "Try to request unknown attribute value: " + std::to_string( attributeId ) );
        m_iZigbee->requestAttributeValue(
            nodeId, endpointId, static_cast< uint16_t >( clusterId ), attributeId, isMfgSpecific, 0x00 );
        return;
    }

    m_iZigbee->requestAttributeValue(
        nodeId, endpointId, static_cast< uint16_t >( clusterId ), attributeId, isMfgSpecific, attribute->getMfgCode() );
}

void ZigbeeAddon::configureAttributeReportInternal( uint16_t nodeId,
                                                    uint8_t endpointId,
                                                    uint16_t clusterId,
                                                    uint16_t attributeId,
                                                    const Json& params,
                                                    bool isMfgSpecific ) {
    auto zigbeeNode = m_nodeManager->findNode( nodeId );

    if( !zigbeeNode ) {
        Log::error( "Try to configure attribute report of unknown node: " + std::to_string( nodeId ) );
        return;
    }

    auto endpoint = zigbeeNode->findEndpoint( endpointId );
    if( !endpoint ) {
        Log::error( "Try to configure attribute report of unknown endpoint: " + std::to_string( endpointId ) );
        return;
    }

    auto cluster = endpoint->findCluster( static_cast< uint16_t >( clusterId ) );
    if( !cluster ) {
        Log::error( "Try to configure attribute report of unknown cluster: " + std::to_string( clusterId ) );
        return;
    }

    auto attribute = cluster->findAttribute( attributeId );
    if( !attribute ) {
        Log::error( "Try to configure unknown attribute report: " + std::to_string( attributeId ) );
        return;
    }

    uint16_t minInterval, maxInterval;
    if( json_get( params, Resources::RPCParams::MinInterval, minInterval ) ) {
        assert( json_get( params, Resources::RPCParams::MaxInterval, maxInterval ) );
        Json reportableChange = params[ Resources::RPCParams::ReportableChange ];

        m_iZigbee->configureAttributeReport( nodeId,
                                             endpointId,
                                             static_cast< uint16_t >( clusterId ),
                                             attributeId,
                                             attribute->getType(),
                                             minInterval,
                                             maxInterval,
                                             reportableChange,
                                             isMfgSpecific,
                                             attribute->getMfgCode() );
    }
    else {
        m_iZigbee->configureAttributeReport( nodeId,
                                             endpointId,
                                             static_cast< uint16_t >( clusterId ),
                                             attributeId,
                                             isMfgSpecific,
                                             attribute->getMfgCode() );
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
