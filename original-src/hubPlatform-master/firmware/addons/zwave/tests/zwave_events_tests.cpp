#include "zwave/script_events/module_reset.h"
#include "zwave/script_events/action_busy.h"
#include "zwave/script_events/node_added.h"
#include "zwave/script_events/node_updated.h"
#include "zwave/script_events/configuration_value_updated.h"
#include "zwave/script_events/association_updated.h"
#include "zwave/script_events/reachable_state_updated.h"
#include "zwave/script_events/value_updated.h"
#include "zwave/script_events/token_value_updated.h"
#include "zwave/script_events/float_value_updated.h"
#include "zwave/script_events/rgb_value_updated.h"
#include "zwave/script_events/node_removed.h"
#include "zwave/script_events/unknown_node_removed.h"
#include "zwave/script_events/s2_select_authentication_modes.h"
#include "zwave/script_events/s2_request_device_specific_key.h"
#include "zwave/script_events/s2_show_device_side_key.h"
#include "zwave/script_events/include_event.h"
#include "zwave/script_events/exclude_event.h"
#include "zwave/script_events/learn_event.h"
#include "zwave/script_events/reconstruct_event.h"
#include "zwave/script_events/reconstruct_progress_error_event.h"
#include "zwave/script_events/reconstruct_progress_event.h"
#include "zwave/script_events/binary_sensor_value_updated.h"
#include "zwave/script_events/thermostat_setpoint_value_updated.h"
#include "zwave/script_events/thermostat_operating_state_value_updated.h"
#include "zwave/script_events/multilevel_sensor_value_updated.h"
#include "zwave/script_events/door_lock_state_updated.h"
#include "zwave/script_events/door_lock_configuration_updated.h"
#include "zwave/script_events/user_code_updated.h"
#include "zwave/script_events/master_code_updated.h"
#include "zwave/script_events/user_codes_checksum_updated.h"
#include "zwave/script_events/user_codes_keypad_mode_updated.h"
#include "zwave/script_events/button_state_updated.h"
#include "zwave/script_events/scene_activated.h"
#include "zwave/script_events/include_device_progress.h"
#include "zwave/script_events/node_firmware_update_status_event.h"
#include "zwave/script_events/node_firmware_download_status_event.h"
#include "zwave/script_events/barrier_signal_value_updated.h"
#include "zwave/script_events/thermostat_setback_value_updated.h"
#include "zwave/script_events/node_check_status.h"
#include "zwave/script_events/sis_node_updated.h"
#include "zwave/script_events/anti_theft_state_updated.h"
#include "zwave/script_events/thermostat_mode_updated.h"
#include "zwave/script_events/vera_migration_finished.h"
#include "zwave/script_events/wake_up_interval_updated.h"
#include "zwave/script_events/simple_av_control_set_received.h"
#include "zwave/script_events/schedule_updated.h"
#include "zwave/script_events/schedule_state_updated.h"
#include "zwave/script_events/value_event.h"

#include "zwave/resources/zwave_resources.h"

#include "json_utils/json_utils.h"

#include <znet/converters/door_lock.hpp>
#include <znet/converters/schedule_converter.hpp>
#include <znet/converters/resources.hpp>
#include <znet/znet.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zwave {
namespace UT {
using namespace Resources::ScriptEvent;

TEST( ZwaveEventToJSON, moduleResetEvent ) {
    ModuleReset event{ znet::ModeStatus::Started, {} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::ModuleReset );
    EXPECT_EQ( joEvent[ Param::ResetStatus ], Param::ActionStatus::Started );
}

TEST( ZwaveEventToJSON, actionBusyEvent ) {
    ActionBusy event{ znet::IZNet::Mode::IncludeStarting };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::ActionBusy );
    EXPECT_EQ( joEvent[ Param::ActionType ], "include_starting" );
}

TEST( ZwaveEventToJSON, nodeAddedEvent ) {
    const auto nodeId = 5;

    NodeAdded event( nodeId );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeAdded );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
}

TEST( ZwaveEventToJSON, nodeUpdatedEvent ) {
    const auto nodeId = 5;

    NodeUpdated event( nodeId );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
}

TEST( ZwaveEventToJSON, reachableStateUpdatedEvent ) {
    const auto nodeId = 5;
    const auto value = true;

    ReachableStateUpdated event( nodeId, value );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 3 );
    EXPECT_EQ( joEvent[ "event" ], Name::ReachableStateUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::Value ], value );
}

TEST( ZwaveEventToJSON, valueUpdatedEvent ) {
    const auto nodeId = 1;
    const auto classId = 2;
    const auto channelId = 3;
    const auto value = 25;

    {
        ValueUpdated event( nodeId, classId, channelId, value, {}, {} );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::ValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ClassId ], classId );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::Value ], value );
    }

    {
        const auto targetValue = 35;
        const std::chrono::seconds duration( 4 );
        ValueUpdated event( nodeId, classId, channelId, value, targetValue, duration );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 7 );
        EXPECT_EQ( joEvent[ "event" ], Name::ValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ClassId ], classId );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::Value ], value );
        EXPECT_EQ( joEvent[ Param::TargetValue ], targetValue );
        EXPECT_EQ( joEvent[ Param::TargetDuration ], duration.count() * 1000 );
    }
}

TEST( ZwaveEventToJSON, tokenValueUpdatedEvent ) {
    const auto nodeId = 1;
    const auto classId = 2;
    const auto channelId = 3;
    const auto value = "token";

    TokenValueUpdated event( nodeId, classId, channelId, value );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::TokenValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], classId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Value ], value );
}

TEST( ZwaveEventToJSON, configurationValueUpdated ) {
    const auto nodeId = 1;
    const auto channelId = 3;
    const auto parameterNumber = 2;
    const auto parameterSize = 4;
    const auto value = 25;

    ConfigurationValueUpdated event( nodeId, channelId, parameterNumber, parameterSize, value );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 7 );
    EXPECT_EQ( joEvent[ "event" ], Name::ConfigurationValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_CONFIGURATION );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::ParameterNum ], parameterNumber );
    EXPECT_EQ( joEvent[ Param::ParameterSize ], parameterSize );
    EXPECT_EQ( joEvent[ Param::Value ], value );
}

TEST( ZwaveEventToJSON, configurationValueUpdatedWithHugeParamNumber ) {
    const auto nodeId = 1;
    const auto channelId = 3;
    const auto parameterNumber = 260;
    const auto parameterSize = 4;
    const auto value = 25;

    ConfigurationValueUpdated event( nodeId, channelId, parameterNumber, parameterSize, value );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 7 );
    EXPECT_EQ( joEvent[ "event" ], Name::ConfigurationValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_CONFIGURATION );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::ParameterNum ], parameterNumber );
    EXPECT_EQ( joEvent[ Param::ParameterSize ], parameterSize );
    EXPECT_EQ( joEvent[ Param::Value ], value );
}

TEST( ZwaveEventToJSON, configurationValueUpdatedWithNegativeValue ) {
    const auto nodeId = 1;
    const auto channelId = 3;
    const auto parameterNumber = 260;
    const auto parameterSize = 4;
    const auto value = 2863311530;
    const auto format = znet::ConfigurationFormat::SignedInteger;

    ConfigurationValueUpdated event( nodeId, channelId, parameterNumber, parameterSize, value, &format );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 7 );
    EXPECT_EQ( joEvent[ "event" ], Name::ConfigurationValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_CONFIGURATION );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::ParameterNum ], parameterNumber );
    EXPECT_EQ( joEvent[ Param::ParameterSize ], parameterSize );
    EXPECT_EQ( joEvent[ Param::Value ], -1431655766 );
}

TEST( ZwaveEventToJSON, associationUpdated ) {
    const auto nodeId = 1;
    const auto channelId = 3;
    const auto classId = ZNET_COMMAND_CLASS_ASSOCIATION;
    const auto groupId = 2;
    std::vector< uint8_t > value{ 1, 2 };

    AssociationUpdated< std::vector< uint8_t > > event( nodeId, channelId, classId, groupId, value );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::AssociationUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_ASSOCIATION );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::GroupId ], groupId );
    const auto& joNodes = joEvent[ Param::GroupNodes ];
    EXPECT_EQ( joNodes.size(), 2 );
    EXPECT_EQ( joNodes[ 0 ][ Param::NodeId ], 1 );
    EXPECT_EQ( joNodes[ 1 ][ Param::NodeId ], 2 );
}

TEST( ZwaveEventToJSON, associationUpdatedMultichannel ) {
    const auto nodeId = 1;
    const auto channelId = 3;
    const auto classId = ZNET_COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION;
    const auto groupId = 2;
    std::vector< std::pair< uint8_t, uint8_t > > value{ { 1, 2 }, { 3, 4 } };

    AssociationUpdated< std::vector< std::pair< uint8_t, uint8_t > > > event(
        nodeId, channelId, classId, groupId, value );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::AssociationUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::GroupId ], groupId );
    const auto& joNodes = joEvent[ Param::GroupNodes ];
    EXPECT_EQ( joNodes.size(), 2 );
    EXPECT_EQ( joNodes[ 0 ][ Param::NodeId ], 1 );
    EXPECT_EQ( joNodes[ 0 ][ Param::EndpointId ], 2 );
    EXPECT_EQ( joNodes[ 1 ][ Param::NodeId ], 3 );
    EXPECT_EQ( joNodes[ 1 ][ Param::EndpointId ], 4 );
}

TEST( ZwaveEventToJSON, floatValueUpdatedEvent ) {
    const auto nodeId = 1;
    const auto classId = 2;
    const auto channelId = 3;
    const auto type = "electric";
    const auto scale = "watt";
    const auto value = 6;
    const auto precision = 7;

    FloatValueUpdated event( nodeId, classId, channelId, type, scale, value, precision );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 8 );
    EXPECT_EQ( joEvent[ "event" ], Name::FloatValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], classId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Type ], type );
    EXPECT_EQ( joEvent[ Param::Scale ], scale );
    EXPECT_EQ( joEvent[ Param::Value ], value );
    EXPECT_EQ( joEvent[ Param::Precision ], precision );
}

TEST( ZwaveEventToJSON, rgbValueUpdatedEvent ) {
    const auto nodeId = 1;
    const auto classId = 2;
    const auto channelId = 3;
    const auto wwhite = 0;
    const auto cwhite = 1;
    const auto red = 2;
    const auto green = 3;
    const auto blue = 4;
    const auto amber = 5;
    const auto cyan = 6;
    const auto purple = 7;

    RgbValueUpdated event( nodeId,
                           classId,
                           channelId,
                           znet::ColorComponentInfos(
                               { { znet::ColorComponentID::Wwhite, wwhite, { 200 }, { std::chrono::seconds( 50 ) } },
                                 { znet::ColorComponentID::Cwhite, cwhite, { 100 }, {} },
                                 { znet::ColorComponentID::Red, red, {}, {} },
                                 { znet::ColorComponentID::Green, green, {}, {} },
                                 { znet::ColorComponentID::Blue, blue, {}, {} },
                                 { znet::ColorComponentID::Amber, amber, {}, {} },
                                 { znet::ColorComponentID::Cyan, cyan, {}, {} },
                                 { znet::ColorComponentID::Purple, purple, {}, {} } } ) );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 7 );
    EXPECT_EQ( joEvent[ "event" ], Name::RgbValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], classId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );

    const auto& joColors = joEvent[ Param::Colors ];
    EXPECT_EQ( joColors.size(), 8 );
    EXPECT_EQ( joColors[ "wwhite" ], wwhite );
    EXPECT_EQ( joColors[ "cwhite" ], cwhite );
    EXPECT_EQ( joColors[ "red" ], red );
    EXPECT_EQ( joColors[ "green" ], green );
    EXPECT_EQ( joColors[ "blue" ], blue );
    EXPECT_EQ( joColors[ "amber" ], amber );
    EXPECT_EQ( joColors[ "cyan" ], cyan );
    EXPECT_EQ( joColors[ "purple" ], purple );

    const auto& joTargetColors = joEvent[ Param::TargetColors ];
    EXPECT_EQ( joTargetColors.size(), 2 );
    EXPECT_EQ( joTargetColors[ "wwhite" ], 200 );
    EXPECT_EQ( joTargetColors[ "cwhite" ], 100 );

    const auto& joTargetDurations = joEvent[ Param::TargetDurations ];
    EXPECT_EQ( joTargetDurations.size(), 1 );
    EXPECT_EQ( joTargetDurations[ "wwhite" ], 50 );
}

TEST( ZwaveEventToJSON, s2SelectAuthenticationModesEvent ) {
    const AuthModeUtils::Modes modes{ Resources::AuthModes::AccessControl, Resources::AuthModes::Authenticated };

    S2SelectAuthenticationModes event( modes );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::S2SelectAuthenticationModes );

    const auto& joModes = joEvent[ Param::Modes ];
    EXPECT_EQ( joModes.size(), modes.size() );
    for( size_t i = 0; i < modes.size(); ++i ) {
        EXPECT_EQ( joModes[ i ], modes[ i ] );
    }
}

TEST( ZwaveEventToJSON, s2RequestDeviceSpecificKeyEvent ) {
    const std::vector< uint16_t > key{ 0x0, 0x1, 0x2 };

    S2RequestDeviceSpecificKey event( key );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::S2RequestDeviceSpecificKey );

    const auto& joKey = joEvent[ Param::Key ];
    EXPECT_EQ( joKey.size(), key.size() );
    for( size_t i = 0; i < key.size(); ++i ) {
        EXPECT_EQ( joKey[ i ], key[ i ] );
    }
}

TEST( ZwaveEventToJSON, s2ShowDeviceSideKeyEvent ) {
    const std::vector< uint16_t > key{ 0x0, 0x1, 0x2 };

    S2ShowDeviceSideKey event( key );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::S2ShowDeviceSideKey );

    const auto& joKey = joEvent[ Param::Key ];
    EXPECT_EQ( joKey.size(), key.size() );
    for( size_t i = 0; i < key.size(); ++i ) {
        EXPECT_EQ( joKey[ i ], key[ i ] );
    }
}

TEST( ZwaveEventToJSON, nodeRemovedEvent ) {
    const auto nodeid = 0;
    const auto manufacturerId = 1;
    const auto productType = 2;
    const auto productId = 3;

    NodeRemoved event( 0, 1, 2, 3 );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeRemoved );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeid );
    EXPECT_EQ( joEvent[ Param::ManufacturerId ], manufacturerId );
    EXPECT_EQ( joEvent[ Param::ProductType ], productType );
    EXPECT_EQ( joEvent[ Param::ProductId ], productId );
}

TEST( ZwaveEventToJSON, unknownNodeRemovedEvent ) {
    UnknownNodeRemoved event;
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], Name::UnknownNodeRemoved );
}

TEST( ZwaveEventToJSON, includeInvokedEvent ) {
    IncludeEvent event{ znet::ModeStatus::Invoked, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "include_invoked" );
}

TEST( ZwaveEventToJSON, includeStartedEvent ) {
    IncludeEvent event{ znet::ModeStatus::Started, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "include_started" );
}

TEST( ZwaveEventToJSON, includeFinishedEvent ) {
    IncludeEvent event{ znet::ModeStatus::Finished, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "include_finished" );
}

TEST( ZwaveEventToJSON, includeFinishedErrorEvent ) {
    IncludeEvent event{ znet::ModeStatus::Failed, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "include_finished_error" );
}

TEST( ZwaveEventToJSON, includeFinishedTimeoutEvent ) {
    IncludeEvent event{ znet::ModeStatus::Timeout, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "include_finished_timeout" );
}

TEST( ZwaveEventToJSON, excludeInvokedEvent ) {
    ExcludeEvent event{ znet::ModeStatus::Invoked, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "exclude_invoked" );
}

TEST( ZwaveEventToJSON, excludeStartedEvent ) {
    ExcludeEvent event{ znet::ModeStatus::Started, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "exclude_started" );
}

TEST( ZwaveEventToJSON, excludeFinishedEvent ) {
    ExcludeEvent event{ znet::ModeStatus::Finished, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "exclude_finished" );
}

TEST( ZwaveEventToJSON, excludeFinishedErrorEvent ) {
    ExcludeEvent event{ znet::ModeStatus::Failed, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "exclude_finished_error" );
}

TEST( ZwaveEventToJSON, excludeFinishedTimeoutEvent ) {
    ExcludeEvent event{ znet::ModeStatus::Timeout, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "exclude_finished_timeout" );
}

TEST( ZwaveEventToJSON, learnInvokedEvent ) {
    LearnEvent event{ znet::ModeStatus::Invoked, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "learn_invoked" );
}

TEST( ZwaveEventToJSON, learnStartedEvent ) {
    LearnEvent event{ znet::ModeStatus::Started, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "learn_started" );
}

TEST( ZwaveEventToJSON, learnFinishedEvent ) {
    LearnEvent event{ znet::ModeStatus::Finished, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "learn_finished" );
}

TEST( ZwaveEventToJSON, learnFinishedErrorEvent ) {
    LearnEvent event{ znet::ModeStatus::Failed, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "learn_finished_error" );
}

TEST( ZwaveEventToJSON, learnFinishedTimeoutEvent ) {
    LearnEvent event{ znet::ModeStatus::Timeout, znet::ErrorOpt{} };
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "learn_finished_timeout" );
}

TEST( ZwaveEventToJSON, binarySensorValueUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;
    const auto classId = ZNET_COMMAND_CLASS_SENSOR_BINARY;
    const auto value = 0;
    const auto sensorType = 5;

    BinarySensorValueUpdated event( nodeId, channelId, value, sensorType );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::BinarySensorValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], classId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Value ], value );
    EXPECT_EQ( joEvent[ Param::SensorType ], sensorType );
}

TEST( ZwaveEventToJSON, thermostatSetpointValueUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;
    const auto classId = ZNET_COMMAND_CLASS_THERMOSTAT_SETPOINT;
    const auto type = 3;
    const auto scale = 4;
    const auto value = 5.5f;

    ThermostatSetpointValueUpdated event{ nodeId, channelId, type, scale, value, true };

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 8 );
    EXPECT_EQ( joEvent[ "event" ], Name::ThermostatSetpointValueUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::ClassId ], classId );
    EXPECT_EQ( joEvent[ Param::Type ], type );
    EXPECT_EQ( joEvent[ Param::Scale ], scale );
    EXPECT_EQ( joEvent[ Param::Value ], value );
    EXPECT_EQ( joEvent[ Param::IsRequested ], true );
}

TEST( ZwaveEventToJSON, doorLockStateUpdatedEvent ) {
    using namespace znet;

    {
        DoorLockStateUpdated event( 2,
                                    0,
                                    DoorLock::Mode::Unknown,
                                    0,
                                    0,
                                    DoorCondition{ LatchState::Open, BoltState::Locked, DoorState::Open },
                                    {},
                                    {},
                                    {} );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 8 );
        EXPECT_EQ( joEvent[ "event" ], Name::DoorLockStateUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 2 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_DOOR_LOCK );
        EXPECT_EQ( joEvent[ Param::DoorLockMode ], door_lock_converter::modeToString( DoorLock::Mode::Unknown ) );
        EXPECT_EQ( joEvent[ Param::EnabledInsideHandleIds ], Json::array() );
        EXPECT_EQ( joEvent[ Param::EnabledOutsideHandleIds ], Json::array() );

        const auto joDoorCondition = joEvent[ Param::DoorCondition ];
        EXPECT_EQ( joDoorCondition.size(), 3 );
        EXPECT_EQ( joDoorCondition[ Param::LatchState ], door_lock_converter::latchStateToString( LatchState::Open ) );
        EXPECT_EQ( joDoorCondition[ Param::BoltState ], door_lock_converter::boltStateToString( BoltState::Locked ) );
        EXPECT_EQ( joDoorCondition[ Param::DoorState ], door_lock_converter::doorStateToString( DoorState::Open ) );
    }

    {
        DoorLockStateUpdated event( 1,
                                    0,
                                    DoorLock::Mode::Secured,
                                    1,
                                    3,
                                    DoorCondition{ LatchState::Open, BoltState::Unlocked, DoorState::Closed },
                                    DoorLockStateUpdated::Seconds( 10 ),
                                    DoorLock::Mode::Unsecured,
                                    DoorLockStateUpdated::Seconds( 6 ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 11 );
        EXPECT_EQ( joEvent[ "event" ], Name::DoorLockStateUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 1 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_DOOR_LOCK );
        EXPECT_EQ( joEvent[ Param::DoorLockMode ], door_lock_converter::modeToString( DoorLock::Mode::Secured ) );
        EXPECT_EQ( joEvent[ Param::LockTimeout ], 10 );
        EXPECT_EQ( joEvent[ Param::TargetDoorLockMode ],
                   door_lock_converter::modeToString( DoorLock::Mode::Unsecured ) );
        EXPECT_EQ( joEvent[ Param::TargetDoorLockModeTimeout ], 6 );
        EXPECT_EQ( joEvent[ Param::EnabledInsideHandleIds ], JSON_ARRAY( 1 ) );
        EXPECT_EQ( joEvent[ Param::EnabledOutsideHandleIds ], JSON_ARRAY( 1 << 2 ) );

        const auto joDoorCondition = joEvent[ Param::DoorCondition ];
        EXPECT_EQ( joDoorCondition.size(), 3 );
        EXPECT_EQ( joDoorCondition[ Param::LatchState ], door_lock_converter::latchStateToString( LatchState::Open ) );
        EXPECT_EQ( joDoorCondition[ Param::BoltState ], door_lock_converter::boltStateToString( BoltState::Unlocked ) );
        EXPECT_EQ( joDoorCondition[ Param::DoorState ], door_lock_converter::doorStateToString( DoorState::Closed ) );
    }

    {
        DoorLockStateUpdated event(
            1,
            0,
            DoorLock::Mode::Secured,
            1,
            3,
            DoorCondition{
                static_cast< LatchState >( 3 ), static_cast< BoltState >( 3 ), static_cast< DoorState >( 3 ) },
            DoorLockStateUpdated::Seconds( 10 ),
            DoorLock::Mode::Unsecured,
            DoorLockStateUpdated::Seconds( 6 ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 11 );
        EXPECT_EQ( joEvent[ "event" ], Name::DoorLockStateUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 1 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_DOOR_LOCK );
        EXPECT_EQ( joEvent[ Param::DoorLockMode ], door_lock_converter::modeToString( DoorLock::Mode::Secured ) );
        EXPECT_EQ( joEvent[ Param::LockTimeout ], 10 );
        EXPECT_EQ( joEvent[ Param::TargetDoorLockMode ],
                   door_lock_converter::modeToString( DoorLock::Mode::Unsecured ) );
        EXPECT_EQ( joEvent[ Param::TargetDoorLockModeTimeout ], 6 );
        EXPECT_EQ( joEvent[ Param::EnabledInsideHandleIds ], JSON_ARRAY( 1 ) );
        EXPECT_EQ( joEvent[ Param::EnabledOutsideHandleIds ], JSON_ARRAY( 1 << 2 ) );

        const auto joDoorCondition = joEvent[ Param::DoorCondition ];
        EXPECT_EQ( joDoorCondition.size(), 3 );
        EXPECT_EQ( joDoorCondition[ Param::LatchState ], "unknown" );
        EXPECT_EQ( joDoorCondition[ Param::BoltState ], "unknown" );
        EXPECT_EQ( joDoorCondition[ Param::DoorState ], "unknown" );
    }
}

TEST( ZwaveEventToJSON, doorLockConfigurationUpdatedEvent ) {
    {
        znet_door_lock_configuration_t configuration{};
        configuration.lock_timeout = 7;
        configuration.are_timed_operations_enabled = true;
        configuration.enabled_inside_handle_ids = 5;
        configuration.enabled_outside_handle_ids = 1;

        DoorLockConfigurationUpdated event(
            1, 0, configuration, DoorLockConfigurationUpdated::ExtendedConfigurationOpt( {}, false ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::DoorLockConfigurationUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 1 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_DOOR_LOCK );

        const auto& joConfiguration = joEvent[ Param::Value ];
        EXPECT_EQ( joConfiguration.size(), 3 );
        EXPECT_EQ( joConfiguration[ Param::LockTimeout ], 7 );
        EXPECT_EQ( joConfiguration[ Param::EnabledInsideHandleIds ], JSON_ARRAY( 1 << 3 ) );
        EXPECT_EQ( joConfiguration[ Param::EnabledOutsideHandleIds ], JSON_ARRAY( 1 ) );
    }

    {
        znet_door_lock_configuration_t configuration{};
        configuration.lock_timeout = 0;
        configuration.are_timed_operations_enabled = false;
        configuration.enabled_inside_handle_ids = 0;
        configuration.enabled_outside_handle_ids = 0;

        znet_door_lock_extended_configuration_t extendedConfiguration;
        extendedConfiguration.auto_relock_timeout = 15;
        extendedConfiguration.hold_and_release_timeout = 22;
        extendedConfiguration.twist_assist_enabled = true;
        extendedConfiguration.block_to_block_enabled = false;

        DoorLockConfigurationUpdated event(
            1,
            0,
            configuration,
            DoorLockConfigurationUpdated::ExtendedConfigurationOpt( extendedConfiguration, true ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::DoorLockConfigurationUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 1 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_DOOR_LOCK );

        const auto& joConfiguration = joEvent[ Param::Value ];
        EXPECT_EQ( joConfiguration.size(), 6 );
        EXPECT_EQ( joConfiguration[ Param::EnabledInsideHandleIds ], Json::array() );
        EXPECT_EQ( joConfiguration[ Param::EnabledOutsideHandleIds ], Json::array() );
        EXPECT_EQ( joConfiguration[ Param::AutoRelockTimeout ], 15 );
        EXPECT_EQ( joConfiguration[ Param::HoldAndReleaseTimeout ], 22 );
        EXPECT_EQ( joConfiguration[ Param::TwistAssistEnabled ], true );
        EXPECT_EQ( joConfiguration[ Param::BlockToBlockEnabled ], false );
    }
}

TEST( ZwaveEventToJSON, userCodeUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;

    {
        UserCodeUpdated event( nodeId,
                               channelId,
                               { { 3, 3, znet::UserCode::UserIDStatus::Available, "4563" } },
                               250,
                               znet::UserCode::UserPollStatus::Polling );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 7 );
        EXPECT_EQ( joEvent[ "event" ], Name::UserCodeUpdated );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_USER_CODE );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ(
            joEvent[ Param::UserCodeRanges ],
            JSON_ARRAY( JSON( Param::UserId << 3 << Param::UserStatus << "available" << Param::UserCode << "4563" ) ) );

        EXPECT_EQ( joEvent[ Param::SupportedUsersCount ], 250 );
        EXPECT_EQ( joEvent[ Param::UserPollingStatus ], "polling" );
    }

    {
        UserCodeUpdated event( nodeId,
                               channelId,
                               { { 4, 10, znet::UserCode::UserIDStatus::Available, "1111" },
                                 { 40, 40, znet::UserCode::UserIDStatus::Enabled, "2222" } },
                               100,
                               znet::UserCode::UserPollStatus::Idle );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 7 );
        EXPECT_EQ( joEvent[ "event" ], Name::UserCodeUpdated );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_USER_CODE );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::UserCodeRanges ],
                   JSON_ARRAY( JSON( Param::UserId << 4 << Param::LastUserId << 10 << Param::UserStatus << "available"
                                                   << Param::UserCode << "1111" )
                               << JSON( Param::UserId << 40 << Param::UserStatus << "enabled" << Param::UserCode
                                                      << "2222" ) ) );

        EXPECT_EQ( joEvent[ Param::SupportedUsersCount ], 100 );
        EXPECT_EQ( joEvent[ Param::UserPollingStatus ], "idle" );
    }
}

TEST( ZwaveEventToJSON, masterCodeUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;
    const auto masterCode = "100500";

    MasterCodeUpdated event( nodeId, channelId, masterCode );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::MasterCodeUpdated );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_USER_CODE );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Value ], masterCode );
}

TEST( ZwaveEventToJSON, userCodesChecksumUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;
    const auto checksum = 61899;

    UserCodesChecksumUpdated event( nodeId, channelId, checksum );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::UserCodesChecksumUpdated );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_USER_CODE );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Value ], checksum );
}

TEST( ZwaveEventToJSON, userCodesKeypadModeUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;

    UserCodesKeypadModeUpdated event( nodeId, channelId, "some_mode" );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::UserCodesKeypadModeUpdated );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_USER_CODE );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Value ], "some_mode" );
}

TEST( ZwaveEventToJSON, buttonStateUpdatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;
    const auto button_num = 3;
    const auto value = "press_1_time";

    ButtonStateUpdated event{ nodeId, channelId, button_num, value };
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::ButtonStateUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_CENTRAL_SCENE );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::ButtonId ], button_num );
    EXPECT_EQ( joEvent[ Param::Value ], value );
}

TEST( ZwaveEventToJSON, sceneActivatedEvent ) {
    const auto nodeId = 1;
    const auto channelId = 2;
    const auto scene_num = 3;

    SceneActivated event{ nodeId, channelId, scene_num };
    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 5 );
    EXPECT_EQ( joEvent[ "event" ], Name::SceneActivated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_SCENE_ACTIVATION );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::SceneId ], scene_num );
}

TEST( ZwaveEventToJSON, includeDeviceProgressEvent ) {
    const auto progress = 50;

    IncludeDeviceProgress event( progress );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::IncludeDeviceProgress );
    EXPECT_EQ( joEvent[ Param::Value ], progress );
}

TEST( ZwaveEventToJSON, nodeFirmwareDownloadStatusEvent ) {
    {
        NodeFirmwareDownloadStatusEvent event( "image", "", "started" );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 3 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareDownloadStatus );
        EXPECT_EQ( joEvent[ Param::Image ], "image" );
        EXPECT_EQ( joEvent[ Param::Status ], "started" );
    }

    {
        NodeFirmwareDownloadStatusEvent event( "image", "checksum", "in_progress", 20 );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareDownloadStatus );
        EXPECT_EQ( joEvent[ Param::Image ], "image" );
        EXPECT_EQ( joEvent[ Param::Checksum ], "checksum" );
        EXPECT_EQ( joEvent[ Param::Status ], "in_progress" );
        EXPECT_EQ( joEvent[ Param::Progress ], 20 );
    }

    {
        NodeFirmwareDownloadStatusEvent event( "image", "checksum", "failed", {}, "some error" );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareDownloadStatus );
        EXPECT_EQ( joEvent[ Param::Image ], "image" );
        EXPECT_EQ( joEvent[ Param::Checksum ], "checksum" );
        EXPECT_EQ( joEvent[ Param::Status ], "failed" );
        EXPECT_EQ( joEvent[ Param::Error ], "some error" );
    }
}

TEST( ZwaveEventToJSON, nodeFirmwareUpdateStatusEvent ) {
    {
        NodeFirmwareUpdateStatusEvent event( 1, {}, "started" );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 3 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareUpdateStatus );
        EXPECT_EQ( joEvent[ Param::NodeId ], 1 );
        EXPECT_EQ( joEvent[ Param::Status ], "started" );
    }

    {
        NodeFirmwareUpdateStatusEvent event( 111, 1, "started" );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 4 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareUpdateStatus );
        EXPECT_EQ( joEvent[ Param::NodeId ], 111 );
        EXPECT_EQ( joEvent[ Param::Target ], 1 );
        EXPECT_EQ( joEvent[ Param::Status ], "started" );
    }

    {
        NodeFirmwareUpdateStatusEvent event( 111, 2, "in_progress", 10 );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareUpdateStatus );
        EXPECT_EQ( joEvent[ Param::NodeId ], 111 );
        EXPECT_EQ( joEvent[ Param::Target ], 2 );
        EXPECT_EQ( joEvent[ Param::Status ], "in_progress" );
        EXPECT_EQ( joEvent[ Param::Progress ], 10 );
    }

    {
        NodeFirmwareUpdateStatusEvent event( 11, 3, "failed", {}, "some error" );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );
        EXPECT_EQ( joEvent[ "event" ], Name::NodeFirmwareUpdateStatus );
        EXPECT_EQ( joEvent[ Param::NodeId ], 11 );
        EXPECT_EQ( joEvent[ Param::Target ], 3 );
        EXPECT_EQ( joEvent[ Param::Status ], "failed" );
        EXPECT_EQ( joEvent[ Param::Error ], "some error" );
    }
}

TEST( ZwaveEventToJSON, reconstructStatusEvent ) {
    {
        ReconstructEvent event( znet::ModeStatus::Started, znet::ErrorOpt{} );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 1 );
        EXPECT_EQ( joEvent[ "event" ], "reconstruct_started" );
    }

    {
        ReconstructEvent event( znet::ModeStatus::Finished, znet::ErrorOpt{} );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 1 );
        EXPECT_EQ( joEvent[ "event" ], "reconstruct_finished" );
    }

    {
        ReconstructEvent event( znet::ModeStatus::Failed, znet::ErrorOpt{} );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 1 );
        EXPECT_EQ( joEvent[ "event" ], "reconstruct_finished_error" );
    }
}

TEST( ZwaveEventToJSON, reconstructProgressEvent ) {
    uint8_t devices = 10;
    uint8_t current = 2;

    ReconstructProgressEvent event( devices, current );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 3 );
    EXPECT_EQ( joEvent[ "event" ], Name::ReconstructProgress );
    EXPECT_EQ( joEvent[ Param::ReconstructDevices ], 10 );
    EXPECT_EQ( joEvent[ Param::CurrentReconstructedDevice ], 2 );
}

TEST( ZwaveEventToJSON, reconstructProgressErrorEvent ) {
    uint8_t nodeId = 10;

    ReconstructProgressErrorEvent event( nodeId );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], Name::ReconstructProgressError );
    EXPECT_EQ( joEvent[ Param::NodeId ], 10 );
}

TEST( ZwaveEventToJSON, barrierSignalValueUpdatedEvent ) {
    {
        const auto nodeId = 1;
        const auto channelId = 2;
        const auto eventNumber = 1;
        const auto value = 0xFF;

        BarrierSignalValueUpdated event{ nodeId, channelId, eventNumber, value };
        const auto joEvent = event.toJson();

        EXPECT_EQ( joEvent.size(), 6 );
        EXPECT_EQ( joEvent[ "event" ], Name::BarrierSignalValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_BARRIER_OPERATOR );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::SignalName ], "audible_notification" );
        EXPECT_EQ( joEvent[ Param::Value ], value );
    }

    {
        const auto nodeId = 1;
        const auto channelId = 2;
        const auto eventNumber = 3;
        const auto value = 0xFF;

        BarrierSignalValueUpdated event{ nodeId, channelId, eventNumber, value };
        const auto joEvent = event.toJson();

        EXPECT_EQ( joEvent.size(), 6 );
        EXPECT_EQ( joEvent[ "event" ], Name::BarrierSignalValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_BARRIER_OPERATOR );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::SignalName ], "unknown" );
        EXPECT_EQ( joEvent[ Param::Value ], value );
    }
}

TEST( ZwaveEventToJSON, thermostatSetbackValueUpdatedTest ) {
    const auto nodeId = 1;
    const auto channelId = 2;

    // without value
    {
        ThermostatSetbackValueUpdated event{ nodeId, channelId, "type", "mode", {} };
        const auto joEvent = event.toJson();

        EXPECT_EQ( joEvent.size(), 6 );
        EXPECT_EQ( joEvent[ "event" ], Name::ThermostatSetbackValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_THERMOSTAT_SETBACK );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::OverrideType ], "type" );
        EXPECT_EQ( joEvent[ Param::Mode ], "mode" );
    }

    // with value
    {
        ThermostatSetbackValueUpdated event{ nodeId, channelId, "a", "b", 10.4f };
        const auto joEvent = event.toJson();

        EXPECT_EQ( joEvent.size(), 7 );
        EXPECT_EQ( joEvent[ "event" ], Name::ThermostatSetbackValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
        EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_THERMOSTAT_SETBACK );
        EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
        EXPECT_EQ( joEvent[ Param::OverrideType ], "a" );
        EXPECT_EQ( joEvent[ Param::Mode ], "b" );
        EXPECT_EQ( joEvent[ Param::Value ], 10.4f );
    }
}

TEST( ZwaveEventToJSON, nodeCheckStatusEvent ) {
    const auto nodeId = 5;
    auto status = znet::ModeStatus::Finished;

    NodeCheckStatus event( nodeId, status );
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 3 );
    EXPECT_EQ( joEvent[ "event" ], Name::NodeCheckStatus );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::Status ], "finished" );
}

TEST( ZwaveEventToJSON, sisNodeUpdatedEvent ) {
    SisNodeUpdated event;
    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], Name::SisNodeUpdated );
}

TEST( ZwaveEventToJSON, antiTheftStateUpdateEvent ) {
    {
        AntiTheftStateUpdated event( 3, 1, 100, {} );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 4 );

        EXPECT_EQ( joEvent[ "event" ], Name::AntiTheftStateUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 3 );
        EXPECT_EQ( joEvent[ Param::ClassId ], 100 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 1 );
    }

    {
        AntiTheftStateUpdated event( 3, 1, 50, znet::AntiTheftInfo( true, { 0xAB, 0x61, 0x0E }, 1900, 789 ) );
        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );

        EXPECT_EQ( joEvent[ "event" ], Name::AntiTheftStateUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 3 );
        EXPECT_EQ( joEvent[ Param::ClassId ], 50 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 1 );
        EXPECT_EQ( joEvent[ Param::LockInfo ],
                   JSON( Param::IsRestricted << true << Param::Hint << "AB610E" << Param::ManufacturerId << 1900
                                             << Param::ZWaveAllianceLockingId << 789 ) );
    }
}

TEST( ZwaveEventToJSON, thermostatModeUpdated ) {
    {
        ThermostatModeUpdated event( 4, 1, 5, "" );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 5 );

        EXPECT_EQ( joEvent[ "event" ], Name::ThermostatModeUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 4 );
        EXPECT_EQ( joEvent[ Param::ClassId ], 64 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 1 );
        EXPECT_EQ( joEvent[ Param::Mode ], 5 );
    }

    {
        ThermostatModeUpdated event( 7, 0, 0x1F, "0286F1" );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 6 );

        EXPECT_EQ( joEvent[ "event" ], Name::ThermostatModeUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 7 );
        EXPECT_EQ( joEvent[ Param::ClassId ], 64 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::Mode ], 0x1F );
        EXPECT_EQ( joEvent[ Param::ManufacturerSpecificData ], "0286F1" );
    }
}

TEST( ZwaveEventToJSON, wakeUpIntervalUpdated ) {
    WakeUpIntervalUpdated event( 15, std::chrono::seconds{ 5400 } );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 3 );
    EXPECT_EQ( joEvent[ "event" ], Name::WakeUpIntervalUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], 15 );
    EXPECT_EQ( joEvent[ Param::Interval ], 5400 );
}

TEST( ZwaveEventToJSON, veraMigrationFinished ) {
    // test no root
    {
        auto veraDevicesModel = Json::parse( R"(
        {
            "devices": [
                {
                    "id": 2,
                    "id_parent" : 1,
                    "altid": "55"
                },
                {
                    "id": 5,
                    "id_parent" : 1,
                    "altid": "12"
                },
                {
                    "id": 8,
                    "id_parent" : 1,
                    "altid": "9"
                }
            ]
        }
        )" );

        auto result = Json::parse( R"(
        {
            "devices": null
        }
        )" );

        VeraMigrationFinished event( std::move( veraDevicesModel ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 2 );

        EXPECT_EQ( joEvent[ "event" ], Name::VeraMigrationFinished );
        EXPECT_EQ( joEvent[ Param::VeraDeviceModel ], result );
    }

    // test root with no children
    {
        auto veraDevicesModel = Json::parse( R"(
            {
                "devices": [
                    {
                        "category_num": 19,
                        "device_type": "urn:schemas-micasaverde-com:device:ZWaveNetwork:1",
                        "id": 1,
                        "invisible": "1"
                    }
                ]
            }
            )" );

        auto result = Json::parse( R"(
            {
                "devices": null
            }
            )" );

        VeraMigrationFinished event( std::move( veraDevicesModel ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 2 );

        EXPECT_EQ( joEvent[ "event" ], Name::VeraMigrationFinished );
        EXPECT_EQ( joEvent[ Param::VeraDeviceModel ], result );
    }

    // test root and several children
    {
        auto veraDevicesModel = Json::parse( R"(
        {
            "devices": [
                {
                    "category_num": 19,
                    "device_type": "urn:schemas-micasaverde-com:device:ZWaveNetwork:1",
                    "id": 1,
                    "invisible": "1"
                },
                {
                    "id": 2,
                    "id_parent" : 1,
                    "altid": "55"
                },
                {
                    "id": 5,
                    "id_parent" : 1,
                    "altid": "12"
                },
                {
                    "id": 8,
                    "id_parent" : 1,
                    "altid": "9"
                }
            ]
        }
        )" );

        auto result = Json::parse( R"(
        {
            "devices": [
                {
                    "id": 2,
                    "altid": "55"
                },
                {
                    "id": 5,
                    "altid": "12"
                },
                {
                    "id": 8,
                    "altid": "9"
                }
            ]
        }
        )" );

        VeraMigrationFinished event( std::move( veraDevicesModel ) );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 2 );

        EXPECT_EQ( joEvent[ "event" ], Name::VeraMigrationFinished );
        EXPECT_EQ( joEvent[ Param::VeraDeviceModel ], result );
    }
}

TEST( ZwaveEventToJSON, simpleAvControlSeted ) {
    SimpleAvControlSetReceived event( 4, 0, 115, "held_down", { 5, 6 } );

    const auto joEvent = event.toJson();
    EXPECT_EQ( joEvent.size(), 7 );

    EXPECT_EQ( joEvent[ "event" ], Name::SimpleAvControlSetReceived );
    EXPECT_EQ( joEvent[ Param::NodeId ], 4 );
    EXPECT_EQ( joEvent[ Param::ClassId ], 148 );
    EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
    EXPECT_EQ( joEvent[ Param::SequenceNumber ], 115 );
    EXPECT_EQ( joEvent[ Param::KeyAttribute ], "held_down" );
    EXPECT_EQ( joEvent[ Param::Commands ], JSON_ARRAY( 5 << 6 ) );
}

TEST( ZwaveEventToJSON, multilevelSensorValueUpdated ) {
    {
        MultilevelSensorValueUpdated event( 4, 0, 115, znet::MultilevelSensor::Type::Illuminance, 0 );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 7 );

        EXPECT_EQ( joEvent[ "event" ], Name::MultilevelSensorValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 4 );
        EXPECT_EQ( joEvent[ Param::ClassId ], 0x31 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::Value ], 115 );
        EXPECT_EQ( joEvent[ Param::SensorType ], "illuminance" );
        EXPECT_EQ( joEvent[ Param::Scale ], "percent" );
    }

    {
        MultilevelSensorValueUpdated event( 4, 0, 115, static_cast< znet::MultilevelSensor::Type >( 1000 ), 5 );

        const auto joEvent = event.toJson();
        EXPECT_EQ( joEvent.size(), 7 );

        EXPECT_EQ( joEvent[ "event" ], Name::MultilevelSensorValueUpdated );
        EXPECT_EQ( joEvent[ Param::NodeId ], 4 );
        EXPECT_EQ( joEvent[ Param::ClassId ], 0x31 );
        EXPECT_EQ( joEvent[ Param::ChannelId ], 0 );
        EXPECT_EQ( joEvent[ Param::Value ], 115 );
        EXPECT_EQ( joEvent[ Param::SensorType ], "unknown" );
        EXPECT_EQ( joEvent[ Param::Scale ], "unknown" );
    }
}

TEST( ZwaveEventToJSON, scheduleUpdated ) {
    const auto nodeId = 2;
    const auto channelId = 0;
    const uint8_t scheduleId = 1;

    using namespace znet;

    const ScheduleIdBlockOpt idBlockOpt{ 3 };
    znet::Schedule value( { .yearday = { 2024, 11, 31 },
                            .weekdays = { ScheduleWeeksDay::Undefined,
                                          ScheduleWeeksDay::Monday,
                                          ScheduleWeeksDay::Undefined,
                                          ScheduleWeeksDay::Undefined,
                                          ScheduleWeeksDay::Undefined,
                                          ScheduleWeeksDay::Friday,
                                          ScheduleWeeksDay::Undefined },
                            .hourminute = { 17, 59 },
                            .options = { .activityStatusOpt = ScheduleActiveStatus::Active,
                                         .recurrenceOffsetOpt = 10,
                                         .recurrenceModeOpt = ScheduleRecurrenceMode::Days,
                                         .relativeOpt = true,
                                         .durationType = ScheduleDurationType::Hours,
                                         .duration = 333 },
                            .commands = { { 0x01, 0x02 }, { 0x03, 0x06, 0x05 }, { 0x06, 0x07 } } } );

    ScheduleUpdated event( nodeId, channelId, scheduleId, idBlockOpt, value );

    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 7 );
    EXPECT_EQ( joEvent[ "event" ], Name::ScheduleUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_SCHEDULE );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    EXPECT_EQ( joEvent[ Param::Schedule::ScheduleId ], scheduleId );
    if( idBlockOpt ) {
        EXPECT_EQ( joEvent[ Param::Schedule::ScheduleIdBlock ], *idBlockOpt );
    }
    const auto& joValue = joEvent[ Param::Value ];
    EXPECT_EQ( joValue.size(), 5 );

    const auto& joHourAndMinute = joValue[ Param::Schedule::Hourandminute ];

    EXPECT_EQ( joHourAndMinute[ Param::Schedule::StartHour ], 17 );
    EXPECT_EQ( joHourAndMinute[ Param::Schedule::StartMinute ], 59 );

    const auto& joYearDay = joValue[ Param::Schedule::Yearday ];

    EXPECT_EQ( joYearDay[ Param::Schedule::StartYear ], 2024 );
    EXPECT_EQ( joYearDay[ Param::Schedule::StartMonth ], 11 );
    EXPECT_EQ( joYearDay[ Param::Schedule::StartDayOfMonth ], 31 );

    const auto& joOptions = joValue[ Param::Schedule::Options ];

    EXPECT_EQ( joOptions[ Param::Schedule::ActivityStatus ], znet::resources::cc_schedule::active_status::Active );
    EXPECT_EQ( joOptions[ Param::Schedule::Duration ], 333 );
    EXPECT_EQ( joOptions[ Param::Schedule::DurationType ], znet::resources::cc_schedule::duration_type::Hours );
    EXPECT_EQ( joOptions[ Param::Schedule::RecurrenceMode ], znet::resources::cc_schedule::recurrence_mode::Days );
    EXPECT_EQ( joOptions[ Param::Schedule::RecurrenceOffset ], 10 );
    EXPECT_EQ( joOptions[ Param::Schedule::Relative ], true );

    const auto& joCommands = joValue[ Param::Schedule::Commands ];
    EXPECT_EQ( joCommands.size(), value.commands.size() );

    std::vector< std::vector< uint8_t > > commands;
    for( const auto& command : joCommands ) {
        std::vector< uint8_t > bytes;
        for( const auto& cmdByte : command ) {
            const auto* ptr = cmdByte.get< const Json::number_integer_t* >();
            if( ptr ) {
                bytes.push_back( *ptr );
            }
        }
        commands.push_back( bytes );
    }

    EXPECT_EQ( value.commands, commands );
}

TEST( ZwaveEventToJSON, scheduleStateUpdated ) {
    const auto nodeId = 2;
    const auto channelId = 0;

    using namespace znet;

    const ScheduleIdBlockOpt idBlockOpt{ 3 };

    std::vector< uint8_t > data{ sizeof( znet_cmd_schedule_state_report_t ) + 10, 0 };
    znet_cmd_schedule_state_report_t* valuePtr =
        static_cast< znet_cmd_schedule_state_report_t* >( static_cast< void* >( data.data() ) );

    znet_cmd_schedule_state_report_t& value = *valuePtr;
    value.number_supported_schedule_ids = 10;
    value.reports_to_follow = 2;
    value.override_support = true;

    for( int i = 0; i < value.number_supported_schedule_ids; ++i ) {
        value.active_id[ i ] = i & 0x07;
    }
    ScheduleStateUpdated event( nodeId, channelId, value, idBlockOpt );

    const auto joEvent = event.toJson();

    EXPECT_EQ( joEvent.size(), 6 );
    EXPECT_EQ( joEvent[ "event" ], Name::ScheduleStateUpdated );
    EXPECT_EQ( joEvent[ Param::NodeId ], nodeId );
    EXPECT_EQ( joEvent[ Param::ClassId ], ZNET_COMMAND_CLASS_SCHEDULE );
    EXPECT_EQ( joEvent[ Param::ChannelId ], channelId );
    if( idBlockOpt ) {
        EXPECT_EQ( joEvent[ Param::Schedule::ScheduleIdBlock ], *idBlockOpt );
    }
    const auto& joValue = joEvent[ Param::Value ];
    EXPECT_EQ( joValue.size(), 4 );

    EXPECT_EQ( joValue[ Param::Schedule::NumberSupportedScheduleIds ], 10 );
    EXPECT_EQ( joValue[ Param::Schedule::ReportsToFollow ], 2 );
    EXPECT_EQ( joValue[ Param::Schedule::OverrideSupport ], true );

    const auto& joStatuses = joValue[ Param::Schedule::ScheduleStasuses ];

    EXPECT_EQ( joStatuses.size(), value.number_supported_schedule_ids );

    int index = 0;
    for( const auto& status : joStatuses ) {
        const auto str = status.get< std::string >();

        EXPECT_NE( str.size(), 0 );

        EXPECT_EQ( std::string( znet::schedule_active_status_converter::activeStatusToString(
                       static_cast< znet::ScheduleActiveStatus >( value.active_id[ index ] ) ) ),
                   str );
        index++;
    }
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
