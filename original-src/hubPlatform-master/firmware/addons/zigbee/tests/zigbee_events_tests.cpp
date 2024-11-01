#include "zigbee/script_events/attribute_value_updated.h"
#include "zigbee/script_events/include_device.h"
#include "zigbee/script_events/include_device_progress.h"
#include "zigbee/script_events/mfg_specific_attribute_value_updated.h"
#include "zigbee/script_events/module_reset.h"
#include "zigbee/script_events/node_added.h"
#include "zigbee/script_events/node_rejoined.h"
#include "zigbee/script_events/node_removed.h"
#include "zigbee/script_events/arm_command_received.h"
#include "zigbee/script_events/simple_command_received.h"
#include "zigbee/script_events/reachable_state_updated.h"
#include "zigbee/script_events/get_user_response_command_received.h"
#include "zigbee/script_events/door_lock_operation_event_notification_received.h"
#include "zigbee/script_events/door_lock_programming_event_notification_received.h"
#include "zigbee/script_events/user_codes_request_progress.h"
#include "zigbee/script_events/alert_received.h"
#include "zigbee/script_events/empty_alerts_list_response_received.h"
#include "zigbee/script_events/child_and_route_info_collected.h"

#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/resources/enums/mode_status.h"

#include "zigbee/model/zigbee_attribute.h"
#include "zigbee/model/enums/commands/user_type.h"
#include "zigbee/model/enums/commands/user_status.h"
#include "zigbee/model/enums/commands/door_lock_event_source.h"
#include "zigbee/model/enums/commands/door_lock_operation_event.h"
#include "zigbee/model/enums/commands/door_lock_programming_event.h"
#include "zigbee/model/enums/commands/alert_notification_cotegory_source.h"
#include "zigbee/model/enums/commands/get_alerts_responce_source.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zigbee {
namespace UT {
using namespace Resources::ScriptEvent;
using namespace ::testing;

class ZigbeeEventToJSON : public Test {
public:
    std::unique_ptr< ZigbeeAttribute > attribute() {
        return ZigbeeAttributeFactory::createZigbeeAttribute( 0x0006, 0x0000, AttributeValueType::BooleanData, false );
    }

    Json renameIdField( Json&& obj ) {
        obj[ "id" ] = obj[ "_id" ];
        obj.erase( "_id" );
        return obj;
    }
};

TEST_F( ZigbeeEventToJSON, moduleResetEvent ) {
    ModuleReset event{ ModeStatus::Started };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::ModuleReset );
    EXPECT_EQ( joEvent[ Param::Status ], Param::ActionStatus::Started );
}

TEST_F( ZigbeeEventToJSON, includeDeviceEvent ) {
    IncludeDevice event{ ModeStatus::Started };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::IncludeDevice );
    EXPECT_EQ( joEvent[ Param::Status ], Param::ActionStatus::Started );
}

TEST_F( ZigbeeEventToJSON, nodeAddedEvent ) {
    const auto nodeId = 5;

    NodeAdded event( nodeId );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeAdded );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
}

TEST_F( ZigbeeEventToJSON, nodeRejoinedEvent ) {
    const auto nodeId = 5;

    NodeRejoined event( nodeId );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeRejoined );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
}

TEST_F( ZigbeeEventToJSON, nodeRemovedEvent ) {
    const auto nodeId = 5;
    const auto manufacturerId = "man_test";
    const auto modelId = "model_test";

    NodeRemoved event( nodeId, manufacturerId, modelId );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 4 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeRemoved );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ManufacturerId ], manufacturerId );
    EXPECT_EQ( joEvent[ Param::ModelId ], modelId );
}

TEST_F( ZigbeeEventToJSON, attributeValueUpdatedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    const auto clusterName = "on_off";
    auto attribute = this->attribute();

    AttributeValueUpdated event( nodeId, endpointId, clusterName, *attribute );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::AttributeValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterName ], clusterName );
    EXPECT_EQ( joEvent[ Param::Attribute ].dump(), renameIdField( attribute->toJson() ).dump() );
}

TEST_F( ZigbeeEventToJSON, mfgSpecificAttributeValueUpdatedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    const auto clusterId = 0x0006;
    auto attribute = this->attribute();

    MfgSpecificAttributeValueUpdated event( nodeId, endpointId, clusterId, *attribute );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::MfgSpecificAttributeValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterId ], clusterId );
    EXPECT_EQ( joEvent[ Param::Attribute ].dump(), renameIdField( attribute->toJson() ).dump() );
}

TEST_F( ZigbeeEventToJSON, armCommandReceivedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    const auto pinCode = "1111";
    const auto mode = IasAceArmCommandMode::Disarm;

    ArmCommandReceived event( nodeId, endpointId, mode, pinCode );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 8 );
    EXPECT_EQ( joEvent[ "event" ], Name::ArmCommandReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterName ], "ias_ace" );
    EXPECT_EQ( joEvent[ Param::CommandId ], 0x00 );
    EXPECT_EQ( joEvent[ Param::CommandName ], "Arm" );
    EXPECT_EQ( joEvent[ Param::ArmMode ], "disarm" );
    EXPECT_EQ( joEvent[ Param::PinCode ], pinCode );
}

TEST_F( ZigbeeEventToJSON, getUserResponseCommandReceivedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    const auto userId = 2;
    UserStatus userStatus = UserStatus::OccupiedEnabled;
    UserType userType = UserType::UnrestrictedUser;
    const auto pinCode = "1111";

    GetUserResponseReceived event( nodeId, endpointId, userId, userStatus, userType, pinCode );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 10 );
    EXPECT_EQ( joEvent[ "event" ], Name::GetUserResponseCommandReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterName ], "door_lock" );
    EXPECT_EQ( joEvent[ Param::CommandId ], 0x06 );
    EXPECT_EQ( joEvent[ Param::CommandName ], "GetPinCodeResponse" );
    EXPECT_EQ( joEvent[ Param::UserId ], userId );
    EXPECT_EQ( joEvent[ Param::UserStatus ], "occupied_enabled" );
    EXPECT_EQ( joEvent[ Param::UserType ], "unrestricted_user" );
    EXPECT_EQ( joEvent[ Param::PinCode ], pinCode );
}

TEST_F( ZigbeeEventToJSON, doorLockOperatingEventNotificationReceivedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    DoorLockEventSource eventSource = DoorLockEventSource::Keypad;
    DoorLockOperationEvent eventType = DoorLockOperationEvent::KeyLock;
    const auto userId = 2;
    const auto pinCode = "1111";

    DoorLockOperatingEventNotificationReceived event( nodeId, endpointId, eventSource, eventType, userId, pinCode );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 10 );
    EXPECT_EQ( joEvent[ "event" ], Name::DoorLockOperatingEventNotificationReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterName ], "door_lock" );
    EXPECT_EQ( joEvent[ Param::CommandId ], 0x20 );
    EXPECT_EQ( joEvent[ Param::CommandName ], "OperatingEventNotification" );
    EXPECT_EQ( joEvent[ Param::UserId ], userId );
    EXPECT_EQ( joEvent[ Param::EventSource ], "keypad" );
    EXPECT_EQ( joEvent[ Param::EventName ], "key_lock" );
    EXPECT_EQ( joEvent[ Param::PinCode ], pinCode );
}

TEST_F( ZigbeeEventToJSON, doorLockProgrammingEventNotificationReceivedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    DoorLockEventSource eventSource = DoorLockEventSource::Keypad;
    DoorLockProgrammingEvent eventType = DoorLockProgrammingEvent::PINCodeAdded;
    const auto userId = 2;
    const auto pinCode = "1111";
    UserStatus userStatus = UserStatus::OccupiedEnabled;
    UserType userType = UserType::UnrestrictedUser;

    DoorLockProgrammingEventNotificationReceived event(
        nodeId, endpointId, eventSource, eventType, userId, pinCode, userType, userStatus );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 12 );
    EXPECT_EQ( joEvent[ "event" ], Name::DoorLockProgrammingEventNotificationReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterName ], "door_lock" );
    EXPECT_EQ( joEvent[ Param::CommandId ], 0x21 );
    EXPECT_EQ( joEvent[ Param::CommandName ], "ProgrammingEventNotification" );
    EXPECT_EQ( joEvent[ Param::UserId ], userId );
    EXPECT_EQ( joEvent[ Param::EventSource ], "keypad" );
    EXPECT_EQ( joEvent[ Param::EventName ], "pin_code_added" );
    EXPECT_EQ( joEvent[ Param::PinCode ], pinCode );
    EXPECT_EQ( joEvent[ Param::UserStatus ], "occupied_enabled" );
    EXPECT_EQ( joEvent[ Param::UserType ], "unrestricted_user" );
}

TEST_F( ZigbeeEventToJSON, simpleCommandReceivedEvent ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    const auto clusterName = "ias_ace";
    const auto commandId = 0x00;
    const auto commandName = "Panic";

    SimpleCommandReceived event( nodeId, endpointId, clusterName, commandId, commandName );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::SimpleCommandReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::ClusterName ], clusterName );
    EXPECT_EQ( joEvent[ Param::CommandId ], commandId );
    EXPECT_EQ( joEvent[ Param::CommandName ], commandName );
}

TEST_F( ZigbeeEventToJSON, reachableStateUpdatedEvent ) {
    const auto nodeId = 5;
    const auto isReachable = true;

    ReachableStateUpdated event( nodeId, isReachable );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 3 );
    EXPECT_EQ( joEvent[ "event" ], Name::ReachableStateUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::Value ], isReachable );
}

TEST_F( ZigbeeEventToJSON, includeDeviceProgressEvent ) {
    const auto progress = 50;

    IncludeDeviceProgress event( progress );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::IncludeDeviceProgress );
    EXPECT_EQ( joEvent[ Param::Value ], progress );
}

TEST_F( ZigbeeEventToJSON, userCodeRequestProgressEvent ) {
    const auto nodeId = 42;
    const auto endpointId = 1;
    const auto progress = 50;

    UserCodesRequestProgress event( nodeId, endpointId, progress );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 4 );
    EXPECT_EQ( joEvent[ "event" ], Name::UserCodesRequestProgress );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::Value ], progress );
}

TEST_F( ZigbeeEventToJSON, alertsNotificationReceived ) {
    const auto nodeId = 5;
    const auto endpointId = 1;
    const AlertStructure alertStructure = { 129,
                                            AlertNotificationCategory::Category::Warning,
                                            AlertNotificationPresenceRecovery::PresenceRecovery::Presence,
                                            0 };
    const char* commandName = "AlertsNotification";
    const AlertSource eventNotificationId = AlertSource::Notification;

    AlertReceived event( nodeId, endpointId, alertStructure, commandName, eventNotificationId );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 10 );
    EXPECT_EQ( joEvent[ "event" ], Name::AlertReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::AlertId ], alertStructure.alert_id );
    EXPECT_EQ( joEvent[ Param::Category ], AlertNotificationCategory::toString( alertStructure.category ) );
    EXPECT_EQ( joEvent[ Param::PresenceRecovery ],
               AlertNotificationPresenceRecovery::toString( alertStructure.presence_recovery ) );
    EXPECT_EQ( joEvent[ Param::ManufacturerSpecificBits ], alertStructure.manufacturer_specific_bits );
    EXPECT_EQ( joEvent[ Param::CommandName ], commandName );
    EXPECT_EQ( joEvent[ Param::CommandId ], 0x01 );
}

TEST_F( ZigbeeEventToJSON, emptyAlertsListResponseReceived ) {
    const auto nodeId = 5;
    const auto endpointId = 1;

    EmptyAlertsListResponseReceived event( nodeId, endpointId );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::EmptyAlertsListReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::EndpointId ], endpointId );
    EXPECT_EQ( joEvent[ Param::CommandName ],
               Resources::Fields::Command::Names::ApplianceEventsAndAlerts::GetAlertsResponse );
    EXPECT_EQ( joEvent[ Param::CommandId ], 0x00 );
}

TEST_F( ZigbeeEventToJSON, childAndRouteInfoCollected_ControllerId ) {
    const auto nodeId = 5;

    ChildAndRouteInfoCollected event( nodeId, {}, {}, {} );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::ChildAndRouteInfoCollected );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::ControllerNodeId ], nodeId );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Children ].empty() );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Neighbors ].empty() );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Routes ].empty() );
}

TEST_F( ZigbeeEventToJSON, childAndRouteInfoCollected_Children ) {
    const auto nodeId = 0;

    std::vector< ChildData > children = {
        ChildData{ 1, ChildData::Type::Unknown, 0 },         ChildData{ 2, ChildData::Type::Coordinator, 1 },
        ChildData{ 3, ChildData::Type::Router, 2 },          ChildData{ 4, ChildData::Type::EndDevice, 3 },
        ChildData{ 5, ChildData::Type::SleepyEndDevice, 4 },
    };

    ChildAndRouteInfoCollected event( nodeId, children, {}, {} );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::ChildAndRouteInfoCollected );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::ControllerNodeId ], nodeId );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Neighbors ].empty() );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Routes ].empty() );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ].size(), 5 );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 0 ].size(), 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 0 ][ Param::NodeId ], 1 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 0 ][ Param::ChildAndRouteInfo::Type ], "unknown" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 0 ][ Param::ChildAndRouteInfo::Timeout ], "10 sec" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 1 ].size(), 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 1 ][ Param::NodeId ], 2 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 1 ][ Param::ChildAndRouteInfo::Type ], "coordinator" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 1 ][ Param::ChildAndRouteInfo::Timeout ], "2 min" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 2 ].size(), 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 2 ][ Param::NodeId ], 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 2 ][ Param::ChildAndRouteInfo::Type ], "router" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 2 ][ Param::ChildAndRouteInfo::Timeout ], "4 min" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 3 ].size(), 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 3 ][ Param::NodeId ], 4 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 3 ][ Param::ChildAndRouteInfo::Type ], "end_device" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 3 ][ Param::ChildAndRouteInfo::Timeout ], "8 min" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 4 ].size(), 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 4 ][ Param::NodeId ], 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 4 ][ Param::ChildAndRouteInfo::Type ],
               "sleepy_end_device" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ][ 4 ][ Param::ChildAndRouteInfo::Timeout ], "16 min" );
}

TEST_F( ZigbeeEventToJSON, childAndRouteInfoCollected_Neighbors ) {
    const auto nodeId = 0;

    std::vector< NeighborData > neighbors = { NeighborData{ 1, 2, 3, 4, 5 }, NeighborData{ 6, 7, 8, 9, 10 } };

    ChildAndRouteInfoCollected event( nodeId, {}, neighbors, {} );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::ChildAndRouteInfoCollected );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::ControllerNodeId ], nodeId );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Children ].empty() );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Routes ].empty() );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ].size(), 2 );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 0 ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 0 ][ Param::NodeId ], 1 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 0 ][ Param::ChildAndRouteInfo::Lqi ], 2 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 0 ][ Param::ChildAndRouteInfo::Age ], 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 0 ][ Param::ChildAndRouteInfo::InCost ], 4 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 0 ][ Param::ChildAndRouteInfo::OutCost ], 5 );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 1 ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 1 ][ Param::NodeId ], 6 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 1 ][ Param::ChildAndRouteInfo::Lqi ], 7 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 1 ][ Param::ChildAndRouteInfo::Age ], 8 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 1 ][ Param::ChildAndRouteInfo::InCost ], 9 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ][ 1 ][ Param::ChildAndRouteInfo::OutCost ], 10 );
}

TEST_F( ZigbeeEventToJSON, childAndRouteInfoCollected_Routes ) {
    const auto nodeId = 0;

    std::vector< RouteData > routes = {
        RouteData{ 1, 2, 3, RouteData::ConcentratorType::Unknown, RouteData::Status::Active },
        RouteData{ 4, 5, 6, RouteData::ConcentratorType::Low, RouteData::Status::Discovering },
        RouteData{ 7, 8, 9, RouteData::ConcentratorType::High, RouteData::Status::Unknown },
        RouteData{ 10, 11, 12, RouteData::ConcentratorType::Unknown, RouteData::Status::Unused }
    };

    ChildAndRouteInfoCollected event( nodeId, {}, {}, routes );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::ChildAndRouteInfoCollected );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::ControllerNodeId ], nodeId );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Children ].empty() );
    EXPECT_TRUE( joEvent[ Param::ChildAndRouteInfo::Neighbors ].empty() );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ].size(), 4 );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 0 ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 0 ][ Param::ChildAndRouteInfo::To ], 1 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 0 ][ Param::ChildAndRouteInfo::Through ], 2 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 0 ][ Param::ChildAndRouteInfo::Age ], 3 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 0 ][ Param::ChildAndRouteInfo::ConcentratorType ],
               "unknown" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 0 ][ Param::Status ], "active" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 1 ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 1 ][ Param::ChildAndRouteInfo::To ], 4 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 1 ][ Param::ChildAndRouteInfo::Through ], 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 1 ][ Param::ChildAndRouteInfo::Age ], 6 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 1 ][ Param::ChildAndRouteInfo::ConcentratorType ], "low" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 1 ][ Param::Status ], "discovering" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 2 ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 2 ][ Param::ChildAndRouteInfo::To ], 7 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 2 ][ Param::ChildAndRouteInfo::Through ], 8 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 2 ][ Param::ChildAndRouteInfo::Age ], 9 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 2 ][ Param::ChildAndRouteInfo::ConcentratorType ], "high" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 2 ][ Param::Status ], "unknown" );

    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 3 ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 3 ][ Param::ChildAndRouteInfo::To ], 10 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 3 ][ Param::ChildAndRouteInfo::Through ], 11 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 3 ][ Param::ChildAndRouteInfo::Age ], 12 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 3 ][ Param::ChildAndRouteInfo::ConcentratorType ],
               "unknown" );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ][ 3 ][ Param::Status ], "unused" );
}

TEST_F( ZigbeeEventToJSON, childAndRouteInfoCollected_Mixed ) {
    const auto nodeId = 0;

    std::vector< ChildData > children = {
        ChildData{ 1, ChildData::Type::Unknown, 0 },         ChildData{ 2, ChildData::Type::Coordinator, 1 },
        ChildData{ 3, ChildData::Type::Router, 2 },          ChildData{ 4, ChildData::Type::EndDevice, 3 },
        ChildData{ 5, ChildData::Type::SleepyEndDevice, 4 },
    };

    std::vector< NeighborData > neighbors = { NeighborData{ 1, 2, 3, 4, 5 }, NeighborData{ 6, 7, 8, 9, 10 } };

    std::vector< RouteData > routes = {
        RouteData{ 1, 2, 3, RouteData::ConcentratorType::Unknown, RouteData::Status::Active },
        RouteData{ 4, 5, 6, RouteData::ConcentratorType::Low, RouteData::Status::Discovering },
        RouteData{ 7, 8, 9, RouteData::ConcentratorType::High, RouteData::Status::Unknown },
        RouteData{ 10, 11, 12, RouteData::ConcentratorType::Unknown, RouteData::Status::Unused }
    };

    ChildAndRouteInfoCollected event( nodeId, children, neighbors, routes );
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::ChildAndRouteInfoCollected );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::ControllerNodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Children ].size(), 5 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Neighbors ].size(), 2 );
    EXPECT_EQ( joEvent[ Param::ChildAndRouteInfo::Routes ].size(), 4 );
}

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
