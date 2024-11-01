#include "zigbee/handlers/handler_monitoring.h"
#include "zigbee/model/enums/commands/ias_ace_arm_command_mode.h"
#include "zigbee/model/enums/commands/user_type.h"
#include "zigbee/model/enums/commands/user_status.h"
#include "zigbee/model/enums/commands/door_lock_event_source.h"
#include "zigbee/model/enums/commands/door_lock_operation_event.h"
#include "zigbee/model/enums/commands/door_lock_programming_event.h"
#include "zigbee/zigbee_task_loop.h"

#include "zigbee/tests/mocks/zigbee_addon_callback_mock.h"
#include "zigbee/tests/mocks/commands_sender_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

    TEST PLAN
DONE    1. Read attribute response
DONE        1.1 Success
DONE            1.1.1 Single
TODO                ... check all attribute types
DONE            1.1.2 Several
DONE        1.2 Failed
DONE            1.2.1 Single
DONE            1.2.2 Several
DONE            1.2.3 One of two with error
DONE    2. Attribute report
DONE        2.1 Single
DONE        2.2 Several
DONE    3. Reachable state updated
DONE        3.1 Reachable
DONE            3.1.1 On attribute response
DONE            3.1.2 On attribute report
DONE            3.1.3 On attribute configure response
DONE            3.1.4 On IAS Zone notification
DONE            3.1.5 On Poll Control Check In
DONE            3.1.6 On IAS Ace Get Panel Status
DONE            3.1.7 On IAS Ace Arm
DONE            3.1.8 On IAS Ace Panic
DONE            3.1.9 On IAS Ace Emergency
DONE            3.1.10 On Default response
DONE            3.1.11 On Door Lock lock response
DONE            3.1.12 On Door Lock unlock response
DONE            3.1.13 On Door Lock set pin response
DONE            3.1.14 On Door Lock clear pin response
DONE            3.1.15 On Door Lock clear all pins response
DONE            3.1.16 On Door Lock get pin response
DONE            3.1.17 On Door Lock operating event notification
DONE            3.1.18 On Door Lock programming event notification
DONE            3.1.19 On Alert Notification or Get Respose Event
DONE        3.2 Unreachable
DONE            3.2.1 On send command
DONE            3.2.2 False on send IAS Ace command
DONE            3.2.3 On set attr value
DONE            3.2.4 On request attr value
DONE            3.2.5 On configure attr report
DONE            3.2.6 On configure attr report with params
DONE            3.2.7 False on send Poll Control command
DONE            3.2.8 False no node
DONE            3.2.9 False node sleepy
DONE    4. Cluster specific callbacks
DONE        4.1 IAS Zone notification
DONE        4.2 IAS Ace Get Panel Status
DONE        4.3 On IAS Ace Arm
DONE        4.4 On IAS Ace Panic
DONE        4.5 On IAS Ace Emergency
DONE        4.6 On Poll Control Check In
DONE        4.7 On Door Lock get pin response
DONE        4.8 On Door Lock operating event notification
DONE        4.9 On Door Lock programming event notification
DONE        4.10 Alerts Notification Or Response Callback
DONE    5. Device controlling
DONE        5.1 Send command
DONE        5.2 Set attribute value
DONE        5.3 Request attribute value
DONE        5.4 Configure attribute report
DONE        5.5 Configure attribute report with params
TODO    6. Device remove


 */

namespace HubPlatform {
namespace Zigbee {

bool operator==( const AlertStructure& first, const AlertStructure& second ) {
    return first.alert_id == second.alert_id && first.category == second.category &&
           first.manufacturer_specific_bits == second.manufacturer_specific_bits &&
           first.presence_recovery == second.presence_recovery;
}

namespace UT {
using namespace Resources::ScriptEvent;
using namespace ::testing;

class ZigbeeMonitoringHandlerFixture : public Test {
protected:
    ZigbeeMonitoringHandlerFixture()
    : m_handlerMonitoring(
          new HandlerMonitoring( m_addonCallbackMock, m_commandsSenderMock, std::chrono::milliseconds{ 50 } ) ) {}

    void SetUp() override {
        ZigbeeTaskLoop::init( []() {} );
    }

    void TearDown() override {
        ZigbeeTaskLoop::stopLoop();
    }

    void waitForReachableStateTimeout() {
        std::this_thread::sleep_for( std::chrono::milliseconds{ 70 } );
    }

    ZigbeeAddonCallbackMock m_addonCallbackMock;
    CommandsSenderMock m_commandsSenderMock;
    std::unique_ptr< HandlerInterface > m_handlerMonitoring;
};

TEST_F( ZigbeeMonitoringHandlerFixture, readAttributeResponseSuccessSingle_1_1_1 ) {
    uint16_t attrId = 0;
    uint8_t value = 123;

    std::vector< uint8_t > data;
    data.push_back( LOW_BYTE( attrId ) );
    data.push_back( HIGH_BYTE( attrId ) );
    data.push_back( EMBER_ZCL_STATUS_SUCCESS );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId, std::to_string( value ) ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->readAttributesResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, readAttributeResponseSuccessSeveral_1_1_2 ) {
    uint16_t attrId1 = 0;
    uint16_t attrId2 = 1;
    uint8_t value1 = 123;
    uint8_t value2 = 124;

    std::vector< uint8_t > data;

    data.push_back( LOW_BYTE( attrId1 ) );
    data.push_back( HIGH_BYTE( attrId1 ) );
    data.push_back( EMBER_ZCL_STATUS_SUCCESS );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value1 );

    data.push_back( LOW_BYTE( attrId2 ) );
    data.push_back( HIGH_BYTE( attrId2 ) );
    data.push_back( EMBER_ZCL_STATUS_SUCCESS );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value2 );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId1, std::to_string( value1 ) ) )
        .Times( Exactly( 1 ) );
    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId2, std::to_string( value2 ) ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->readAttributesResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, readAttributeResponseFailSingle_1_2_1 ) {
    uint16_t attrId = 0;

    std::vector< uint8_t > data;
    data.push_back( LOW_BYTE( attrId ) );
    data.push_back( HIGH_BYTE( attrId ) );
    data.push_back( EMBER_ZCL_STATUS_FAILURE );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( _, _, _, _, _ ) ).Times( Exactly( 0 ) );

    m_handlerMonitoring->readAttributesResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, readAttributeResponseFailSeveral_1_2_2 ) {
    uint16_t attrId1 = 0;
    uint16_t attrId2 = 1;

    std::vector< uint8_t > data;
    // First attr
    data.push_back( LOW_BYTE( attrId1 ) );
    data.push_back( HIGH_BYTE( attrId1 ) );
    data.push_back( EMBER_ZCL_STATUS_FAILURE );
    // Second attr
    data.push_back( LOW_BYTE( attrId2 ) );
    data.push_back( HIGH_BYTE( attrId2 ) );
    data.push_back( EMBER_ZCL_STATUS_FAILURE );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( _, _, _, _, _ ) ).Times( Exactly( 0 ) );

    m_handlerMonitoring->readAttributesResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, readAttributeResponseFailOneOfTwo_1_2_3 ) {
    uint16_t attrId1 = 0;
    uint16_t attrId2 = 1;
    uint8_t value2 = 124;

    std::vector< uint8_t > data;
    // First attr
    data.push_back( LOW_BYTE( attrId1 ) );
    data.push_back( HIGH_BYTE( attrId1 ) );
    data.push_back( EMBER_ZCL_STATUS_FAILURE );
    // Second attr
    data.push_back( LOW_BYTE( attrId2 ) );
    data.push_back( HIGH_BYTE( attrId2 ) );
    data.push_back( EMBER_ZCL_STATUS_SUCCESS );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value2 );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId2, std::to_string( value2 ) ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->readAttributesResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, attributeReportSingle_2_1 ) {
    uint16_t attrId = 0;
    uint8_t value = 123;

    std::vector< uint8_t > data;
    data.push_back( LOW_BYTE( attrId ) );
    data.push_back( HIGH_BYTE( attrId ) );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId, std::to_string( value ) ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->reportAttributesCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, attributeReportSeveral_2_2 ) {
    uint16_t attrId1 = 0;
    uint16_t attrId2 = 1;
    uint8_t value1 = 123;
    uint8_t value2 = 124;

    std::vector< uint8_t > data;

    data.push_back( LOW_BYTE( attrId1 ) );
    data.push_back( HIGH_BYTE( attrId1 ) );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value1 );

    data.push_back( LOW_BYTE( attrId2 ) );
    data.push_back( HIGH_BYTE( attrId2 ) );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value2 );

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId1, std::to_string( value1 ) ) )
        .Times( Exactly( 1 ) );
    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 1, 1, attrId2, std::to_string( value2 ) ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->reportAttributesCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnAttributeResponse_3_1_1 ) {
    uint16_t attrId = 0;
    uint8_t value = 123;

    std::vector< uint8_t > data;
    data.push_back( LOW_BYTE( attrId ) );
    data.push_back( HIGH_BYTE( attrId ) );
    data.push_back( EMBER_ZCL_STATUS_SUCCESS );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value );

    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->readAttributesResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnAttributeReport_3_1_2 ) {
    uint16_t attrId = 0;
    uint8_t value = 123;

    std::vector< uint8_t > data;
    data.push_back( LOW_BYTE( attrId ) );
    data.push_back( HIGH_BYTE( attrId ) );
    data.push_back( static_cast< uint8_t >( AttributeValueType::Uint8Data ) );
    data.push_back( value );

    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->reportAttributesCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnAttributeConfigureResponse_3_1_3 ) {
    std::vector< uint8_t > data;

    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->configureReportingResponseCallback( 1, 1, 1, &data.front(), data.size() );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnIasZoneNotification_3_1_4 ) {
    HandlerInterface::DeviceData data{ 1, 1, 1, 0 };

    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( data, 0, 0, 0, 0 );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnPollControlCheckIn_3_1_5 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->pollControlCheckInCallback( 1, 1, ZCL_POLL_CONTROL_CLUSTER_ID );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnIasAceGetPanelStatus_3_1_6 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAceGetPanelStatusReceivedCallback( 1, 1, ZCL_IAS_ACE_CLUSTER_ID );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnIasAceArm_3_1_7 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAceArmReceivedCallback(
        1, 1, static_cast< uint8_t >( IasAceArmCommandMode::Disarm ), "1111" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnIasAcePanic_3_1_8 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAcePanicReceivedCallback( 1, 1 );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnIasAceEmergency_3_1_9 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAceEmergencyReceivedCallback( 1, 1 );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDefaultResponse_3_1_10 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->defaultResponseCallback( 1, 1, 1, 1, EMBER_ZCL_STATUS_SUCCESS );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockLockResponse_3_1_11 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockLockDoorResponse( 1, 1, EMBER_ZCL_STATUS_SUCCESS );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockUnlockResponse_3_1_12 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockUnlockDoorResponse( 1, 1, EMBER_ZCL_STATUS_SUCCESS );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockSetPinResponse_3_1_13 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockSetPinResponse( 1, 1, EMBER_ZCL_STATUS_SUCCESS );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockClearPinResponse_3_1_14 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockClearPinResponse( 1, 1, EMBER_ZCL_STATUS_SUCCESS );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockClearAllPinsResponse_3_1_15 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockClearAllPinsResponse( 1, 1, EMBER_ZCL_STATUS_SUCCESS );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockGetPinResponse_3_1_16 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockGetPinResponse( 1, 1, 2, 0, 0, "1111" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockOperatingEventNotification_3_1_17 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockOperatingEventNotification( 1, 1, 0, 0, 2, "1111", 0, "" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnDoorLockProgrammingEventNotification_3_1_17 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockProgrammingEventNotification( 1, 1, 0, 0, 2, "1111", 0, 0, 0, "" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedReachableOnAlertNotificationOrGetResposeEvent_3_1_19 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, true ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->alertsNotificationOrResponseCallback(
        1,
        1,
        { { 1,
            AlertNotificationCategory::Category::Warning,
            AlertNotificationPresenceRecovery::PresenceRecovery::Presence,
            1 } },
        HubPlatform::Zigbee::AlertSource::Response );
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableOnSendCommand_3_2_1 ) {
    m_addonCallbackMock.addNode( 1 );
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->sendCommand( 1, 1, 1, "On", JSON_EMPTY_OBJ );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableFalseOnSendIasAceCommand_3_2_2 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 0 ) );

    m_handlerMonitoring->sendCommand( 1, 1, ZCL_IAS_ACE_CLUSTER_ID, "On", JSON_EMPTY_OBJ );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableOnSetAttrValue_3_2_3 ) {
    m_addonCallbackMock.addNode( 1 );
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->setAttributeValue( 1, 1, 1, 1, AttributeValueType::Uint8Data, JSON_EMPTY_OBJ );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableOnRequestAttrValue_3_2_4 ) {
    m_addonCallbackMock.addNode( 1 );
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->requestAttributeValue( 1, 1, 1, 1, false, 1 );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableOnConfigureAttrReport_3_2_5 ) {
    m_addonCallbackMock.addNode( 1 );
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->configureAttributeReport( 1, 1, 1, 1, false, 1 );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableOnConfigureAttrReportWithParams_3_2_6 ) {
    m_addonCallbackMock.addNode( 1 );
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->configureAttributeReport(
        1, 1, 1, 1, AttributeValueType::Uint8Data, 1, 1, JSON_EMPTY_OBJ, false, 1 );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableFalseOnSendPollControlCommand_3_2_7 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 0 ) );

    m_handlerMonitoring->sendCommand( 1, 1, ZCL_POLL_CONTROL_CLUSTER_ID, "FastPollStop", JSON_EMPTY_OBJ );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableFalseNoNode_3_2_8 ) {
    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 0 ) );

    m_handlerMonitoring->sendCommand( 1, 1, 1, "On", JSON_EMPTY_OBJ );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, reachableStateUpdatedUnreachableFalseSleepyNode_3_2_9 ) {
    m_addonCallbackMock.addNode( 1 );
    m_addonCallbackMock.takeNode( 1 )->setCapabilities( 0 );

    EXPECT_CALL( m_addonCallbackMock, updateDeviceReachable( 1, false ) ).Times( Exactly( 0 ) );

    m_handlerMonitoring->sendCommand( 1, 1, 1, "On", JSON_EMPTY_OBJ );
    waitForReachableStateTimeout();
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksIasZoneNotification_4_1 ) {
    HandlerInterface::DeviceData data{ 1, 2, 3, 4 };

    EXPECT_CALL( m_addonCallbackMock, attributeValueUpdated( 1, 2, 3, 4, std::to_string( 5 ) ) ).Times( Exactly( 1 ) );

    m_handlerMonitoring->hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( data, 5, 0, 0, 0 );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksIasAceGetPanelStatus_4_2 ) {
    EXPECT_CALL( m_addonCallbackMock,
                 simpleCommandReceived( 1, 2, ZCL_IAS_ACE_CLUSTER_ID, ZCL_GET_PANEL_STATUS_COMMAND_ID ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAceGetPanelStatusReceivedCallback( 1, 2, ZCL_IAS_ACE_CLUSTER_ID );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksIasAceArm_4_3 ) {
    EXPECT_CALL( m_addonCallbackMock, armCommandReceived( 1, 2, IasAceArmCommandMode::Disarm, "1111" ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAceArmReceivedCallback(
        1, 2, static_cast< uint8_t >( IasAceArmCommandMode::Disarm ), "1111" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksIasAcePanic_4_4 ) {
    EXPECT_CALL( m_addonCallbackMock, simpleCommandReceived( 1, 2, ZCL_IAS_ACE_CLUSTER_ID, ZCL_PANIC_COMMAND_ID ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAcePanicReceivedCallback( 1, 2 );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksIasAceEmergency_4_5 ) {
    EXPECT_CALL( m_addonCallbackMock, simpleCommandReceived( 1, 2, ZCL_IAS_ACE_CLUSTER_ID, ZCL_EMERGENCY_COMMAND_ID ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->iasAceEmergencyReceivedCallback( 1, 2 );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksPollControlCheckIn_4_6 ) {
    EXPECT_CALL( m_addonCallbackMock,
                 simpleCommandReceived( 1, 2, ZCL_POLL_CONTROL_CLUSTER_ID, ZCL_CHECK_IN_COMMAND_ID ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->pollControlCheckInCallback( 1, 2, ZCL_POLL_CONTROL_CLUSTER_ID );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksDoorLockGetPinResponse_4_7 ) {
    EXPECT_CALL(
        m_addonCallbackMock,
        doorLockGetUserResponseReceived( 1, 2, 3, UserStatus::OccupiedEnabled, UserType::UnrestrictedUser, "1111" ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockGetPinResponse( 1, 2, 3, 0x01, 0x00, "1111" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksDoorLockOperatingEventNotification_4_8 ) {
    EXPECT_CALL( m_addonCallbackMock,
                 doorLockOperatingEventNotificationReceived(
                     1, 2, DoorLockEventSource::Keypad, DoorLockOperationEvent::LockFailureInvalidPINorID, 3, "1111" ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockOperatingEventNotification( 1, 2, 0x00, 0x03, 3, "1111", 0, "" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksDoorLockProgrammingEventNotification_4_9 ) {
    EXPECT_CALL( m_addonCallbackMock,
                 doorLockProgrammingEventNotificationReceived( 1,
                                                               2,
                                                               DoorLockEventSource::Keypad,
                                                               DoorLockProgrammingEvent::PINCodeAdded,
                                                               3,
                                                               "1111",
                                                               UserType::UnrestrictedUser,
                                                               UserStatus::OccupiedEnabled ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->doorLockProgrammingEventNotification( 1, 2, 0x00, 0x02, 3, "1111", 0x00, 0x01, 0, "" );
}

TEST_F( ZigbeeMonitoringHandlerFixture, clusterSpecificCallbacksAlertsNotificationOrResponseCallback_4_10 ) {
    AlertStructure a;
    a.alert_id = 1;
    a.category = AlertNotificationCategory::Category::Warning;
    a.manufacturer_specific_bits = 1;
    a.presence_recovery = AlertNotificationPresenceRecovery::PresenceRecovery::Presence;
    std::vector< AlertStructure > t{ a };

    EXPECT_CALL( m_addonCallbackMock,
                 alertsNotificationOrResponseReceived( 1, 1, t, HubPlatform::Zigbee::AlertSource::Response ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->alertsNotificationOrResponseCallback( 1, 1, t, HubPlatform::Zigbee::AlertSource::Response );
}

TEST_F( ZigbeeMonitoringHandlerFixture, deviceControllingSendCommand_5_1 ) {
    auto nodeId = 1;
    auto endpointId = 2;
    auto clusterId = 3;
    auto commandName = "testCommand";
    Json commandParams = { "testParam1", 1, "testParam2", 2 };

    EXPECT_CALL( m_commandsSenderMock, sendCommand( nodeId, endpointId, clusterId, commandName, commandParams ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->sendCommand( nodeId, endpointId, clusterId, commandName, commandParams );
}

TEST_F( ZigbeeMonitoringHandlerFixture, deviceControllingSetAttrValue_5_2 ) {
    auto nodeId = 1;
    auto endpointId = 2;
    auto clusterId = 3;
    auto attrId = 4;
    AttributeValueType attrValueType = AttributeValueType::Uint8Data;
    Json attrValue = { "5" };

    EXPECT_CALL(
        m_commandsSenderMock,
        setAttributeValue( nodeId, endpointId, clusterId, attrId, static_cast< uint8_t >( attrValueType ), attrValue ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->setAttributeValue( nodeId, endpointId, clusterId, attrId, attrValueType, attrValue );
}

TEST_F( ZigbeeMonitoringHandlerFixture, deviceControllingRequestAttrValue_5_3 ) {
    auto nodeId = 1;
    auto endpointId = 2;
    auto clusterId = 3;
    auto attrId = 4;
    auto isMfgSpecific = false;
    auto mfgCode = 5;

    EXPECT_CALL( m_commandsSenderMock,
                 requestAttribute( nodeId, endpointId, clusterId, attrId, isMfgSpecific, mfgCode ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->requestAttributeValue( nodeId, endpointId, clusterId, attrId, isMfgSpecific, mfgCode );
}

TEST_F( ZigbeeMonitoringHandlerFixture, deviceControllingConfigureAttrReport_5_4 ) {
    auto nodeId = 1;
    auto endpointId = 2;
    auto clusterId = 3;
    auto attrId = 4;
    auto isMfgSpecific = false;
    auto mfgCode = 5;

    EXPECT_CALL(
        m_commandsSenderMock,
        sendConfigureAttributeReportingCommand( nodeId, endpointId, clusterId, attrId, isMfgSpecific, mfgCode ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->configureAttributeReport( nodeId, endpointId, clusterId, attrId, isMfgSpecific, mfgCode );
}

TEST_F( ZigbeeMonitoringHandlerFixture, deviceControllingConfigureAttrReportWithParams_5_5 ) {
    auto nodeId = 1;
    auto endpointId = 2;
    auto clusterId = 3;
    auto attrId = 4;
    AttributeValueType attrValueType = AttributeValueType::Uint8Data;
    auto minInterval = 5;
    auto maxInterval = 6;
    Json reportableChange = { "7" };
    auto isMfgSpecific = false;
    auto mfgCode = 8;

    EXPECT_CALL( m_commandsSenderMock,
                 sendConfigureAttributeReportingCommand( nodeId,
                                                         endpointId,
                                                         clusterId,
                                                         attrId,
                                                         attrValueType,
                                                         minInterval,
                                                         maxInterval,
                                                         reportableChange,
                                                         isMfgSpecific,
                                                         mfgCode ) )
        .Times( Exactly( 1 ) );

    m_handlerMonitoring->configureAttributeReport( nodeId,
                                                   endpointId,
                                                   clusterId,
                                                   attrId,
                                                   attrValueType,
                                                   minInterval,
                                                   maxInterval,
                                                   reportableChange,
                                                   isMfgSpecific,
                                                   mfgCode );
}

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
