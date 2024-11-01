#include "zigbee/zigbee_interface.h"
#include "zigbee/zigbee_task_loop.h"
#include "zigbee/resources/enums/mode.h"
#include "zigbee/resources/enums/mode_status.h"
#include "zigbee/backup/zigbee_network_info.h"

#include "zigbee/interview_configurations/node_interview_configurations.h"
#include "zigbee/interview_configurations/node_interview_names_extractors.h"

#include "zigbee/utils/child_and_route_info_extractor.h"

#include "common_utils/cpp14.h"
#include "database_client/dbqueryparser.h"
#include "zigbee/stack_wrapper.h"
#include "zigbee/zigbee_log.h"
#include "log_utils/ezlo_log.h"

extern const char* __progname;
const JsonArray jDefaultInitParameters{ "-b", "115200", "-f", "x", "-p", "/dev/zbstick" };
const std::vector< std::string > defaultParameters = jDefaultInitParameters;
const int maxInitParamsCount = 7;

namespace {
constexpr const std::chrono::seconds includingTimeoutSeconds( 60 );
}

namespace HubPlatform {
namespace Zigbee {

ZigbeeInterface::ZigbeeInterface( ZigbeeAddonCallbackInterface& callbackInterface )
: m_callbackInterface( callbackInterface )
, m_includeTimer( [this]() {
    Log::debug( "Inclusion timeout reached" );
    addZigbeeTask( [this]() { includeStatusChange( ModeStatus::Timeout ); } );
} )
, m_monitoringHandler( m_callbackInterface, m_commandsSender )
, m_backupManager( *this )
, m_mode( Mode::Idle )
, m_modeStatus( ModeStatus::Undefined ) {}

IZigbeeResult ZigbeeInterface::startInclude() {
    addZigbeeTask( [this]() {
        m_includeTimer.start( includingTimeoutSeconds, true );

        ZigbeeStackWrapper::permitJoining();
        includeStatusChange( ModeStatus::Invoked );
    } );
    return IZigbeeResult::SUCCESS;
}

IZigbeeResult ZigbeeInterface::stopInclude() {
    addZigbeeTask( [this]() {
        if( m_mode == Mode::Including &&
            ( m_modeStatus == ModeStatus::Invoked || m_modeStatus == ModeStatus::Started ) ) {
            includeStatusChange( ModeStatus::Finished );
        }
        else {
            Log::print( Log::PriorityLevel::ERROR,
                        "Unable to stop include. mode = %s, status = %s",
                        toString( m_mode ),
                        toString( m_modeStatus ) );
        }
    } );
    return IZigbeeResult::SUCCESS;
}

IZigbeeResult ZigbeeInterface::removeNode( EmberNodeId nodeId ) {
    addZigbeeTask( [this, nodeId]() {
        Log::print( Log::PriorityLevel::DEBUG, "Zigbee remove node: nodeId: 0x%04x", nodeId );
        sendLeaveCommand( nodeId );
    } );
    return IZigbeeResult::SUCCESS;
}

IZigbeeResult ZigbeeInterface::resetToDefault() {
    addZigbeeTask( []() {
        Log::debug( "Zigbee reset" );
        ZigbeeStackWrapper::resetToDefaults();
    } );
    return IZigbeeResult::SUCCESS;
}

IZigbeeResult ZigbeeInterface::startNewNetwork() {
    addZigbeeTask( []() { ZigbeeStackWrapper::startNewNetwork(); } );
    return IZigbeeResult::SUCCESS;
}

void ZigbeeInterface::collectNetworkInfo() {
    addZigbeeTask( [this]() { m_backupManager.collectNetworkInfo(); } );
}

void ZigbeeInterface::getNetworkInfo() {
    addZigbeeTask( [this]() {
        Backup::ZigbeeNetworkInfo info;

        bool completed = false;
        EmberStatus getStatus;
        EmberNodeType nodeType;
        EmberNetworkParameters params;

        getStatus = emberAfGetNetworkParameters( &nodeType, &params );
        if( getStatus == EMBER_SUCCESS ) {
            info.panId = params.panId;
            info.nodeId = (unsigned int) EMBER_TRUST_CENTER_NODE_ID;
            info.radioChannel = params.radioChannel;
            info.radioPower = params.radioTxPower;
            MEMCOPY( &( info.ePanId ), &( params.extendedPanId ), sizeof( params.extendedPanId ) );
            EmberEUI64 value;
            emberLookupEui64ByNodeId( EMBER_TRUST_CENTER_NODE_ID, &value[ 0 ] );
            MEMCOPY( &( info.eui64 ), &( value ), sizeof( value ) );
            completed = true;
        }
        else {
            Log::error( cc::error( "Failed to retrieve node information to save" ) );
            completed = false;
        }

        m_callbackInterface.networkInfoCollected( info, completed );
    } );
}

void ZigbeeInterface::restoreBackup( const Backup::ZigbeeNetworkInfo& info ) {
    addZigbeeTask( [this, info]() { m_backupManager.restoreNetwork( info ); } );
}

void ZigbeeInterface::sendCommand( uint16_t nodeId,
                                   uint8_t endpointId,
                                   uint16_t clusterId,
                                   const std::string& commandName,
                                   const Json& commandParams ) {
    addZigbeeTask( [this, nodeId, endpointId, clusterId, commandName, commandParams]() {
        getHandler( nodeId ).sendCommand( nodeId, endpointId, clusterId, commandName, commandParams );
    } );
}

void ZigbeeInterface::setAttributeValue( uint16_t nodeId,
                                         uint8_t endpointId,
                                         uint16_t clusterId,
                                         uint16_t attributeId,
                                         AttributeValueType attributeValueType,
                                         const Json& value ) {
    addZigbeeTask( [this, nodeId, endpointId, clusterId, attributeId, attributeValueType, value]() {
        getHandler( nodeId ).setAttributeValue( nodeId, endpointId, clusterId, attributeId, attributeValueType, value );
    } );
}

void ZigbeeInterface::setNetworkRadioChannel( const uint8_t value ) {
    addZigbeeTask( [this, value]() { ezspSetRadioChannel( value ); } );
}

void ZigbeeInterface::getChildAndRouteInfo() {
    addZigbeeTask( [this]() { ChildAndRouteInfoExtractor( *this ).extract(); } );
}

void ZigbeeInterface::requestAttributeValue( uint16_t nodeId,
                                             uint8_t endpointId,
                                             uint16_t clusterId,
                                             uint16_t attributeId,
                                             bool isMfgSpecific,
                                             uint16_t mfgCode ) {
    addZigbeeTask( [this, nodeId, endpointId, clusterId, attributeId, isMfgSpecific, mfgCode]() {
        getHandler( nodeId ).requestAttributeValue(
            nodeId, endpointId, clusterId, attributeId, isMfgSpecific, mfgCode );
    } );
}

void ZigbeeInterface::requestUserCodes( uint16_t nodeId,
                                        uint8_t endpointId,
                                        uint16_t startUserId,
                                        uint16_t endUserId ) {
    addZigbeeTask( [this, nodeId, endpointId, startUserId, endUserId]() {
        auto& handler = initiateUserCodesRequestHandler( nodeId, endpointId );
        handler.start( startUserId, endUserId );
    } );
}

void ZigbeeInterface::configureAttributeReport( uint16_t nodeId,
                                                uint8_t endpointId,
                                                uint16_t clusterId,
                                                uint16_t attributeId,
                                                bool isMfgSpecific,
                                                uint16_t mfgCode ) {
    addZigbeeTask( [this, nodeId, endpointId, clusterId, attributeId, isMfgSpecific, mfgCode]() {
        getHandler( nodeId ).configureAttributeReport(
            nodeId, endpointId, clusterId, attributeId, isMfgSpecific, mfgCode );
    } );
}

void ZigbeeInterface::configureAttributeReport( uint16_t nodeId,
                                                uint8_t endpointId,
                                                uint16_t clusterId,
                                                uint16_t attributeId,
                                                AttributeValueType attributeValueType,
                                                const uint16_t minInterval,
                                                const uint16_t maxInterval,
                                                const Json& reportableChange,
                                                bool isMfgSpecific,
                                                uint16_t mfgCode ) {
    addZigbeeTask( [this,
                    nodeId,
                    endpointId,
                    clusterId,
                    attributeId,
                    attributeValueType,
                    minInterval,
                    maxInterval,
                    reportableChange,
                    isMfgSpecific,
                    mfgCode]() {
        getHandler( nodeId ).configureAttributeReport( nodeId,
                                                       endpointId,
                                                       clusterId,
                                                       attributeId,
                                                       attributeValueType,
                                                       minInterval,
                                                       maxInterval,
                                                       reportableChange,
                                                       isMfgSpecific,
                                                       mfgCode );
    } );
}

IZigbeeResult ZigbeeInterface::init( const std::vector< std::string > initParameters ) {
    Log::debug( "Start Zigbee interface initialization" );

    char* argVector[ maxInitParamsCount ];
    prepareInitParameters( argVector, initParameters );

    int result = ZigbeeStackWrapper::init( maxInitParamsCount, (char**) &argVector );

    if( ZigbeeStackWrapper::RESULT_SUCCESS == result ) {
        m_callbackInterface.stackInitialized();
    }

    return ( result == ZigbeeStackWrapper::RESULT_SUCCESS ? IZigbeeResult::SUCCESS : IZigbeeResult::FAILED );
}

void ZigbeeInterface::resetAndInitNCP() {
    ZigbeeStackWrapper::resetAndInitNCP();
}

void ZigbeeInterface::mainLoop() {
    ZigbeeStackWrapper::mainLoop();
}

void ZigbeeInterface::addZigbeeTask( ZigbeeInterface::ZigbeeTask&& task ) {
    ZigbeeTaskLoop::getInstance().addTask( task );
}

void ZigbeeInterface::sendLeaveCommand( EmberNodeId nodeId ) {
    getHandler( nodeId ).sendLeaveCommand( nodeId );
}

void ZigbeeInterface::prepareInitParameters( char** target, const std::vector< std::string > source ) const {
    target[ 0 ] = (char*) __progname;

    std::vector< std::string > parameters( source );
    if( parameters.empty() ) {
        parameters = defaultParameters;
    }

    int index = 1;
    for( const auto& arg : parameters ) {
        if( index < maxInitParamsCount ) {
            target[ index ] = (char*) arg.c_str();
            ++index;
        }
    }
}

// stack callback interface
bool ZigbeeInterface::stackStatusCallback( EmberStatus status ) {
    ZB_LOG_DEBUG( "%s: status = %d\n", __func__, status );
    if( EMBER_NETWORK_DOWN == status ) {
        Log::debug( "EMBER_NETWORK_DOWN status received\n" );
        networkDown();
    }
    else if( EMBER_NETWORK_UP == status ) {
        Log::debug( "EMBER_NETWORK_UP status received\n" );
        ZigbeeStackWrapper::getNetworkParameters();
        networkUp();
    }
    else if( EMBER_CHANNEL_CHANGED == status ) {
        Log::debug( "EMBER_CHANNEL_CHANGED status received\n" );
        onNetworkChannelChanged();
    }

    return false;
}

void ZigbeeInterface::deviceRejoined( EmberNodeId nodeId ) {
    // Nothing to do
}

void ZigbeeInterface::trustCenterJoinCallback( EmberNodeId nodeId,
                                               uint8_t* newNodeEui64,
                                               EmberNodeId parentOfNewNode,
                                               EmberDeviceUpdate status,
                                               EmberJoinDecision decision ) {
    if( EMBER_NO_ACTION == decision &&
        ( EMBER_STANDARD_SECURITY_SECURED_REJOIN == status || EMBER_STANDARD_SECURITY_UNSECURED_REJOIN == status ) ) {
        Log::print( Log::PriorityLevel::DEBUG, "Skip rejoining of existing device: 0x%04x / %u\n", nodeId, nodeId );
        getHandler( nodeId ).deviceRejoined( nodeId );
        return;
    }

    if( status == EMBER_DEVICE_LEFT ) {
        getHandler( nodeId ).trustCenterJoinCallback( nodeId, newNodeEui64, parentOfNewNode, status, decision );
    }
    else if( m_mode == Mode::Including ) {
        auto& handler = initiateInterviewHandler( nodeId );
        handler.trustCenterJoinCallback( nodeId, newNodeEui64, parentOfNewNode, status, decision );
    }
}

// TODO remove it
bool ZigbeeInterface::preMessageReceivedCallback( EmberAfIncomingMessage* incomingMessage ) {
    return false;
}

// TODO remove it
bool ZigbeeInterface::messageSentCallback( EmberOutgoingMessageType type,
                                           uint16_t indexOrDestination,
                                           EmberApsFrame* apsFrame,
                                           uint16_t msgLen,
                                           uint8_t* message,
                                           EmberStatus status ) {
    // TODO investigate how to use this callback and its status for retransmitions if send failed
    // ZB_LOG_DEBUG( "%s: status - 0x%02x, seq - 0x%02x\n", __PRETTY_FUNCTION__, status, apsFrame->sequence );
    return false;
}

// TODO remove it
bool ZigbeeInterface::preZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                                     EmberApsFrame* apsFrame,
                                                     uint8_t* message,
                                                     uint16_t length ) {
    getHandler( emberNodeId ).preZDOMessageReceivedCallback( emberNodeId, apsFrame, message, length );
    return false;
}

bool ZigbeeInterface::readAttributesResponseCallback( EmberNodeId emberNodeId,
                                                      EmberAfClusterId clusterId,
                                                      uint8_t endpointId,
                                                      uint8_t* buffer,
                                                      uint16_t bufLen ) {
    return getHandler( emberNodeId )
        .readAttributesResponseCallback( emberNodeId, clusterId, endpointId, buffer, bufLen );
}

bool ZigbeeInterface::reportAttributesCallback( EmberNodeId emberNodeId,
                                                EmberAfClusterId clusterId,
                                                uint8_t endpointId,
                                                uint8_t* buffer,
                                                uint16_t bufLen ) {
    return getHandler( emberNodeId ).reportAttributesCallback( emberNodeId, clusterId, endpointId, buffer, bufLen );
}

bool ZigbeeInterface::attributesDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                           EmberAfClusterId clusterId,
                                                           uint8_t endpointId,
                                                           uint8_t* buffer,
                                                           uint16_t bufLen,
                                                           bool extended ) {
    return getHandler( emberNodeId )
        .attributesDiscoveryResponseCallback( emberNodeId, clusterId, endpointId, buffer, bufLen, extended );
}

bool ZigbeeInterface::commandsDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                         EmberAfClusterId clusterId,
                                                         uint8_t endpointId,
                                                         CommandType commandsType,
                                                         uint8_t* commandIds,
                                                         uint16_t commandIdCount ) {
    return getHandler( emberNodeId )
        .commandsDiscoveryResponseCallback(
            emberNodeId, clusterId, endpointId, commandsType, commandIds, commandIdCount );
}

bool ZigbeeInterface::defaultResponseCallback( EmberNodeId emberNodeId,
                                               EmberAfClusterId clusterId,
                                               uint8_t endpointId,
                                               uint8_t commandId,
                                               EmberAfStatus status ) {
    return getHandler( emberNodeId ).defaultResponseCallback( emberNodeId, clusterId, endpointId, commandId, status );
}

void ZigbeeInterface::childJoined( uint16_t nodeId, bool joining, NodeType childType ) {
    getHandler( nodeId ).childJoined( nodeId, joining, childType );
}

void ZigbeeInterface::discoveryCompleteCallback( const EmberAfDeviceInfo* device ) {
    const EmberNodeId nodeId = emberLookupNodeIdByEui64( ( (EmberAfDeviceInfo*) device )->eui64 );
    getHandler( nodeId ).discoveryCompleteCallback( device );
}

void ZigbeeInterface::configureReportingResponseCallback( uint16_t nodeId,
                                                          uint8_t endpointId,
                                                          uint16_t clusterId,
                                                          uint8_t* buffer,
                                                          uint16_t bufLen ) {
    getHandler( nodeId ).configureReportingResponseCallback( nodeId, endpointId, clusterId, buffer, bufLen );
}

// attribute specific stack callback interface
void ZigbeeInterface::hubPlatformAttributeNotificationCallbackUint8( DeviceData devData, uint8_t value ) {
    getHandler( devData.nodeId ).hubPlatformAttributeNotificationCallbackUint8( devData, value );
}

void ZigbeeInterface::hubPlatformAttributeNotificationCallbackUint16( DeviceData devData, uint16_t value ) {
    getHandler( devData.nodeId ).hubPlatformAttributeNotificationCallbackUint8( devData, value );
}

// cluster specific stack callback interface
void ZigbeeInterface::hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( DeviceData devData,
                                                                                     uint16_t zoneStatus,
                                                                                     uint8_t extendedStatus,
                                                                                     uint8_t zoneId,
                                                                                     uint16_t delay ) {
    LOGGER_DEBUG_PRINT(
        "Ias Zone Status Change Notification: node_id: %u, endpoint_id: %u, cluster_id: %u, attribute_id: %u, "
        "zone: %u, status: 0x%04x",
        devData.nodeId,
        devData.endpointId,
        devData.clusterId,
        devData.attributeId,
        zoneId,
        zoneStatus );

    getHandler( devData.nodeId )
        .hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback(
            devData, zoneStatus, extendedStatus, zoneId, delay );
}

void ZigbeeInterface::pollControlCheckInCallback( EmberNodeId emberNodeId,
                                                  uint8_t endpointId,
                                                  EmberAfClusterId clusterId ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Poll Control Check In received: nodeId 0x%04x, endpointId 0x%02x",
                emberNodeId,
                endpointId );

    getHandler( emberNodeId ).pollControlCheckInCallback( emberNodeId, endpointId, clusterId );
}

void ZigbeeInterface::iasAceGetPanelStatusReceivedCallback( EmberNodeId emberNodeId,
                                                            uint8_t endpointId,
                                                            EmberAfClusterId clusterId ) {
    Log::debug( "IasZone.GetPanelStatus command received" );

    getHandler( emberNodeId ).iasAceGetPanelStatusReceivedCallback( emberNodeId, endpointId, clusterId );
}

void ZigbeeInterface::iasAceArmReceivedCallback( EmberNodeId emberNodeId,
                                                 uint8_t endpointId,
                                                 uint8_t armMode,
                                                 const std::string& armDisarmCode ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "IAS Ace Arm received: nodeId 0x%04x, endpointId 0x%02x, pinCode %s",
                emberNodeId,
                endpointId,
                armDisarmCode.c_str() );

    getHandler( emberNodeId ).iasAceArmReceivedCallback( emberNodeId, endpointId, armMode, armDisarmCode );
}

void ZigbeeInterface::iasAcePanicReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "IAS Ace Panic received: nodeId 0x%04x, endpointId 0x%02x",
                emberNodeId,
                endpointId );

    getHandler( emberNodeId ).iasAcePanicReceivedCallback( emberNodeId, endpointId );
}

void ZigbeeInterface::iasAceEmergencyReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "IAS Ace Emergency received: nodeId 0x%04x, endpointId 0x%02x",
                emberNodeId,
                endpointId );

    getHandler( emberNodeId ).iasAceEmergencyReceivedCallback( emberNodeId, endpointId );
}

void ZigbeeInterface::doorLockLockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    Log::print(
        Log::PriorityLevel::DEBUG, "Door lock lock response: nodeId 0x%04x, status 0x%02x", emberNodeId, status );
    getHandler( emberNodeId ).doorLockLockDoorResponse( emberNodeId, endpointId, status );
}

void ZigbeeInterface::doorLockUnlockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    Log::print(
        Log::PriorityLevel::DEBUG, "Door lock unlock response: nodeId 0x%04x, status 0x%02x", emberNodeId, status );
    getHandler( emberNodeId ).doorLockUnlockDoorResponse( emberNodeId, endpointId, status );
}

void ZigbeeInterface::doorLockSetPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    Log::print(
        Log::PriorityLevel::DEBUG, "Door lock set pin response: nodeId 0x%04x, status 0x%02x", emberNodeId, status );
    getHandler( emberNodeId ).doorLockSetPinResponse( emberNodeId, endpointId, status );
}

void ZigbeeInterface::doorLockClearPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    Log::print(
        Log::PriorityLevel::DEBUG, "Door lock clear pin response: nodeId 0x%04x, status 0x%02x", emberNodeId, status );
    getHandler( emberNodeId ).doorLockClearPinResponse( emberNodeId, endpointId, status );
}

void ZigbeeInterface::doorLockClearAllPinsResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Door lock clear all pins response: nodeId 0x%04x, status 0x%02x",
                emberNodeId,
                status );
    getHandler( emberNodeId ).doorLockClearAllPinsResponse( emberNodeId, endpointId, status );
}

void ZigbeeInterface::doorLockGetPinResponse( EmberNodeId emberNodeId,
                                              uint8_t endpointId,
                                              uint16_t userId,
                                              uint8_t userStatus,
                                              uint8_t userType,
                                              const std::string& pin ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Door lock get pin response: nodeId 0x%04x, userId 0x%02x, userStatus 0x%02x, userType 0x%02x, pin %s",
                emberNodeId,
                userId,
                userStatus,
                userType,
                pin.c_str() );

    getHandler( emberNodeId ).doorLockGetPinResponse( emberNodeId, endpointId, userId, userStatus, userType, pin );
}

void ZigbeeInterface::doorLockOperatingEventNotification( EmberNodeId emberNodeId,
                                                          uint8_t endpointId,
                                                          uint8_t source,
                                                          uint8_t eventCode,
                                                          uint16_t userId,
                                                          const std::string& pin,
                                                          uint32_t timeStamp,
                                                          const std::string& data ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Door lock operation event notification: nodeId 0x%04x, source 0x%02x, eventCode 0x%02x, userId "
                "0x%02x, pin %s, data %s",
                emberNodeId,
                source,
                eventCode,
                userId,
                pin.c_str(),
                data.c_str() );

    getHandler( emberNodeId )
        .doorLockOperatingEventNotification( emberNodeId, endpointId, source, eventCode, userId, pin, timeStamp, data );
}

void ZigbeeInterface::doorLockProgrammingEventNotification( EmberNodeId emberNodeId,
                                                            uint8_t endpointId,
                                                            uint8_t source,
                                                            uint8_t eventCode,
                                                            uint16_t userId,
                                                            const std::string& pin,
                                                            uint8_t userType,
                                                            uint8_t userStatus,
                                                            uint32_t timeStamp,
                                                            const std::string& data ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Door lock operation event notification: nodeId 0x%04x, source 0x%02x, eventCode 0x%02x, userId "
                "0x%02x, pin %s, userType 0x%02x, userStatus 0x%02x, data %s",
                emberNodeId,
                source,
                eventCode,
                userId,
                pin.c_str(),
                userType,
                userStatus,
                data.c_str() );

    getHandler( emberNodeId )
        .doorLockProgrammingEventNotification(
            emberNodeId, endpointId, source, eventCode, userId, pin, userType, userStatus, timeStamp, data );
}

const ZigbeeNode* ZigbeeInterface::findNode( uint16_t nodeId ) {
    return std::move( m_callbackInterface.findNode( nodeId ) );
}

void ZigbeeInterface::stackInitialized() {
    m_callbackInterface.stackInitialized();
}

// addon callback interface
void ZigbeeInterface::nodeAdded( std::shared_ptr< ZigbeeNode > zigbeeNode ) {
    auto nodeId = zigbeeNode->getId();

    m_callbackInterface.nodeAdded( zigbeeNode );
    addZigbeeTask( [this, nodeId]() { m_interviewHandlers.erase( nodeId ); } );
}

void ZigbeeInterface::nodeRemoved( const uint16_t zigbeeNode ) {
    m_callbackInterface.nodeRemoved( zigbeeNode );
}

void ZigbeeInterface::nodeRejoined( uint16_t nodeId ) {
    m_callbackInterface.nodeRejoined( nodeId );
}

void ZigbeeInterface::includeStatusChange( ModeStatus status ) {
    Log::info( cc::info( std::string( "Include process changed status : " ) + toString( status ) ) );

    changeMode( Mode::Including, status );

    switch( status ) {
    case ModeStatus::Finished:
    case ModeStatus::Failed:
    case ModeStatus::Timeout:
        addZigbeeTask( [this]() {
            m_interviewHandlers.clear();
            stopIncludeInternal();

            changeMode( Mode::Idle, ModeStatus::Undefined );
        } );
        break;

    case ModeStatus::Started:
        addZigbeeTask( [this]() { m_includeTimer.stop(); } );
    }

    m_callbackInterface.includeStatusChange( status );
}

void ZigbeeInterface::attributeValueUpdated( uint16_t nodeId,
                                             uint16_t endpointId,
                                             uint16_t clusterId,
                                             uint16_t attributeId,
                                             const std::string value ) {
    m_callbackInterface.attributeValueUpdated( nodeId, endpointId, clusterId, attributeId, value );
}

void ZigbeeInterface::armCommandReceived( uint16_t nodeId,
                                          uint16_t endpointId,
                                          IasAceArmCommandMode mode,
                                          const std::string pinCode ) {
    m_callbackInterface.armCommandReceived( nodeId, endpointId, mode, pinCode );
}

void ZigbeeInterface::simpleCommandReceived( uint16_t nodeId,
                                             uint16_t endpointId,
                                             uint16_t clusterId,
                                             uint16_t commandId ) {
    m_callbackInterface.simpleCommandReceived( nodeId, endpointId, clusterId, commandId );
}

void ZigbeeInterface::doorLockGetUserResponseReceived( uint16_t nodeId,
                                                       uint16_t endpointId,
                                                       uint16_t userId,
                                                       UserStatus userStatus,
                                                       UserType userType,
                                                       const std::string& pinCode ) {
    m_callbackInterface.doorLockGetUserResponseReceived( nodeId, endpointId, userId, userStatus, userType, pinCode );
}

void ZigbeeInterface::doorLockOperatingEventNotificationReceived( uint16_t nodeId,
                                                                  uint16_t endpointId,
                                                                  DoorLockEventSource eventSource,
                                                                  DoorLockOperationEvent event,
                                                                  uint16_t userId,
                                                                  const std::string& pinCode ) {
    m_callbackInterface.doorLockOperatingEventNotificationReceived(
        nodeId, endpointId, eventSource, event, userId, pinCode );
}

void ZigbeeInterface::doorLockProgrammingEventNotificationReceived( uint16_t nodeId,
                                                                    uint16_t endpointId,
                                                                    DoorLockEventSource eventSource,
                                                                    DoorLockProgrammingEvent event,
                                                                    uint16_t userId,
                                                                    const std::string& pinCode,
                                                                    UserType userType,
                                                                    UserStatus userStatus ) {
    m_callbackInterface.doorLockProgrammingEventNotificationReceived(
        nodeId, endpointId, eventSource, event, userId, pinCode, userType, userStatus );
}

void ZigbeeInterface::alertsNotificationOrResponseReceived(
    uint16_t nodeId,
    uint8_t endpointId,
    const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
    HubPlatform::Zigbee::AlertSource type ) {
    m_callbackInterface.alertsNotificationOrResponseReceived( nodeId, endpointId, alert_structures, type );
}

void ZigbeeInterface::updateDeviceReachable( uint16_t nodeId, bool isReachable ) {
    m_callbackInterface.updateDeviceReachable( nodeId, isReachable );
}

void ZigbeeInterface::includeDeviceProgress( uint8_t progress ) {
    m_callbackInterface.includeDeviceProgress( progress );
}

void ZigbeeInterface::userCodesRequestProgress( uint16_t nodeId, uint16_t endpointId, uint8_t progress ) {
    m_callbackInterface.userCodesRequestProgress( nodeId, endpointId, progress );

    if( progress == 100 ) {
        addZigbeeTask( [this, nodeId]() { m_userCodesRequestHandlers.erase( nodeId ); } );
    }
}

void ZigbeeInterface::networkDown() {
    if( m_backupManager.isBackupRestoreInProgress() ) {
        m_backupManager.onNetworkDown();
    }
    else {
        m_callbackInterface.networkDown();
    }
}

void ZigbeeInterface::networkUp() {
    m_callbackInterface.networkUp();
}

void ZigbeeInterface::onNetworkChannelChanged() {
    m_callbackInterface.onNetworkChannelChanged();
}

void ZigbeeInterface::onChildAndRouteInfo( uint16_t selfNodeId,
                                           std::vector< ChildData > children,
                                           std::vector< NeighborData > neighbors,
                                           std::vector< RouteData > routes ) {
    m_callbackInterface.onChildAndRouteInfo(
        selfNodeId, std::move( children ), std::move( neighbors ), std::move( routes ) );
}

void ZigbeeInterface::networkInfoCollected( const Backup::ZigbeeNetworkInfo& info, bool completed ) {
    m_callbackInterface.networkInfoCollected( info, completed );
}

void ZigbeeInterface::restoreBackupCompleted() {
    m_callbackInterface.restoreBackupCompleted();
}

void ZigbeeInterface::restoreBackupFailed( Backup::BackupRestoreStatus reason ) {
    m_callbackInterface.restoreBackupFailed( reason );
}

std::unique_ptr< NodeInterviewConfigurations > ZigbeeInterface::provideNodeInterviewConfigurations(
    const std::string& manufacturerId,
    const std::string& modelId ) {
    return std::move( m_callbackInterface.provideNodeInterviewConfigurations( manufacturerId, modelId ) );
}

std::unique_ptr< NodeInterviewNamesExtractors > ZigbeeInterface::provideNodeInterviewNamesExtractors() {
    return std::move( m_callbackInterface.provideNodeInterviewNamesExtractors() );
}

HandlerInterface& ZigbeeInterface::initiateInterviewHandler( EmberNodeId nodeId ) {
    if( m_interviewHandlers.find( nodeId ) == m_interviewHandlers.end() ) {
        m_interviewHandlers[ nodeId ] = std::make_shared< HandlerInterview >( *this, m_commandsSender );
    }

    return *m_interviewHandlers[ nodeId ];
}

HandlerUserCodesRequest& ZigbeeInterface::initiateUserCodesRequestHandler( uint16_t nodeId, uint8_t endpointId ) {
    if( m_userCodesRequestHandlers.find( nodeId ) == m_userCodesRequestHandlers.end() ) {
        m_userCodesRequestHandlers[ nodeId ] =
            std::make_shared< HandlerUserCodesRequest >( *this, m_commandsSender, nodeId, endpointId );
    }

    return *m_userCodesRequestHandlers[ nodeId ];
}

HandlerInterface& ZigbeeInterface::getHandler( EmberNodeId nodeId ) {
    if( m_interviewHandlers.find( nodeId ) != m_interviewHandlers.end() )
        return *m_interviewHandlers[ nodeId ];

    if( m_userCodesRequestHandlers.find( nodeId ) != m_userCodesRequestHandlers.end() )
        return *m_userCodesRequestHandlers[ nodeId ];

    return m_monitoringHandler;
}

void ZigbeeInterface::stopIncludeInternal() {
    m_includeTimer.stop();

    ZigbeeStackWrapper::permitJoining( 0 );
    Log::debug( "Zigbee including stopped" );
}

void ZigbeeInterface::changeMode( Mode mode, ModeStatus status ) {
    m_mode = mode;
    m_modeStatus = status;
}

void ZigbeeInterface::alertsNotificationOrResponseCallback(
    EmberNodeId emberNodeId,
    uint8_t endpointId,
    const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
    HubPlatform::Zigbee::AlertSource type ) {
    LOGGER_DEBUG_PRINT(
        "%s received: nodeId 0x%04x, endpointId 0x%02x",
        type == HubPlatform::Zigbee::AlertSource::Notification ? "Alerts Notification" : "Get Alert Response",
        emberNodeId,
        endpointId );

    getHandler( emberNodeId ).alertsNotificationOrResponseCallback( emberNodeId, endpointId, alert_structures, type );
}

}  // namespace Zigbee
}  // namespace HubPlatform
