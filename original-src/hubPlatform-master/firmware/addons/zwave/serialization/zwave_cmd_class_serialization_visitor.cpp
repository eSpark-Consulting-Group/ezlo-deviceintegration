#include "zwave/serialization/zwave_cmd_class_serialization_visitor.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_all_command_classes.hpp>

#include <znet/converters/thermostat_operating_state_converter.hpp>
#include <znet/converters/multilevel_sensor_converter.hpp>
#include <znet/converters/central_scene_attributes_converter.hpp>
#include <znet/converters/barrier_operator_signal_converter.hpp>
#include <znet/converters/configuration_format_converter.hpp>
#include <znet/converters/signed_configuration_parameter_converter.hpp>
#include <znet/converters/color_switch_converter.hpp>
#include <znet/converters/door_lock.hpp>
#include <znet/converters/user_code.hpp>
#include <znet/converters/agi_profile_converter.hpp>
#include <znet/converters/agi_commands_converter.hpp>
#include <znet/converters/agi_commands_converter.hpp>
#include <znet/converters/schedule_converter.hpp>

#include <znet/znet_notification.hpp>
#include <znet/znet_alarm_sensor.hpp>
#include <znet/znet_meter.hpp>

#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Zwave {

namespace {

template < typename T >
void addOptionalField( JsonBuilder& builder, const char* name, znet::utils::optional< T > value ) {
    if( value ) {
        builder << name << std::move( *value );
    }
}

}  // namespace

using namespace znet;
using namespace Resources::Fields;

void ZwaveCmdClassSerializationVisitor::operator()( const ZClass& cc ) {
    m_jsonBuilder << Class::Id << cc.getId() << Class::Version << cc.getVersion();
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x2BSceneActivation& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x2CSceneActuatorConf& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x5BCentralScene& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto buttons = Json::array();
    for( uint8_t i = 1; i <= cc.getSceneNum(); ++i ) {
        auto attributes = central_scenes_attributes_converter::maskToStrings( cc.getAttribute( i ) );
        if( !attributes.empty() ) {
            JsonObject button;
            button[ Class::CentralSceneButtonId ] = i;
            button[ Class::CentralSceneButtonAttributes ] = JsonObject( attributes );

            buttons.push_back( std::move( button ) );
        }
    }

    m_jsonBuilder << Class::CentralSceneButtons << std::move( buttons );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x6FEntryControl& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x20Basic& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    m_jsonBuilder << Class::IsSupported << cc.isSupported();
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x25Switch& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x26SwitchMultiLevel& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x63UserCode& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    m_jsonBuilder << Class::SupportedUsersCount << cc.getSupportedUsersCount();

    if( const auto capabilities = cc.getCapabilities() ) {
        auto jsonCapabilities = Json::object();

        jsonCapabilities[ Class::ChecksumSupported ] = capabilities->isCheckSumSupported;
        jsonCapabilities[ Class::MasterCodeSupported ] = capabilities->isMasterCodeSupported;
        jsonCapabilities[ Class::MasterCodeDeactivationSupported ] = capabilities->isMasterCodeDeactivationSupported;
        jsonCapabilities[ Class::SupportedKeys ] =
            std::string( capabilities->supportedKeys.begin(), capabilities->supportedKeys.end() );

        Json& jsonKeypadModes = jsonCapabilities[ Class::SupportedKeypadModes ] = Json::array();
        for( auto mode : capabilities->supportedKeypadModes ) {
            if( const char* stringMode = user_code_converter::keypadModeToString( mode ) ) {
                jsonKeypadModes.push_back( stringMode );
            }
            else {
                LOGGER_WARN( "Cannot recognize user code key mode " +
                             std::to_string( static_cast< uint8_t >( mode ) ) );
            }
        }

        Json& jsonUserIdStatuses = jsonCapabilities[ Class::SupportedUserIdStatuses ] = Json::array();
        for( const auto status : capabilities->supportedUserIdStatuses ) {
            if( const char* stringStatus = user_code_converter::userIdStatusToString( status ) ) {
                jsonUserIdStatuses.push_back( stringStatus );
            }
            else {
                LOGGER_WARN( "Cannot recognize user code status " +
                             std::to_string( static_cast< uint8_t >( status ) ) );
            }
        }
        m_jsonBuilder << Class::Capabilities << std::move( jsonCapabilities );
    }
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x30BinarySensor& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    m_jsonBuilder << Class::SensorTypes << cc.getSensorTypes();
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x31SensorMultilevel& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto sensors = Json::array();
    for( const auto& info : cc.getSupportedSensorsInfo() ) {
        JsonObject sensor;
        const char* sensorTypeStr = znet::multilevel_sensor_converter::typeToString( info.type );
        sensor[ Class::SensorType ] = sensorTypeStr ? sensorTypeStr : "Unknown";

        auto& scales = sensor[ Class::Scales ] = Json::array();

        for( auto scale : info.scales ) {
            const char* scaleString = znet::multilevel_sensor_converter::scaleToString( info.type, scale );
            if( scaleString != nullptr ) {
                scales.push_back( scaleString );
            }
        }

        sensors.push_back( std::move( sensor ) );
    }

    m_jsonBuilder << Class::Sensors << std::move( sensors );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x32Meter& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
    JsonObject scales;
    const size_t scalesCount{ cc.getScaleCount() };
    for( size_t i{ 0 }; i < scalesCount; ++i ) {
        if( const auto scale = znet::meter::scaleToString( cc.getType(), cc.getScale( i ) ) ) {
            scales.push_back( scale );
        }
        else {
            LOGGER_WARN( "Cannot recognize meter scale " + std::to_string( cc.getScale( i ) ) );
        }
    }

    const auto type = znet::meter::typeToString( cc.getType() );

    if( type ) {
        m_jsonBuilder << Class::Type << type;
    }
    else {
        LOGGER_WARN( "Meter type " + std::to_string( static_cast< int >( cc.getType() ) ) + " is unknown" );
    }

    m_jsonBuilder << Class::ResetSupported << cc.isResetSupported() << Class::Scales << std::move( scales );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x33ColorSwitch& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto componentIds = Json::array();
    for( const auto componentId : cc.getSupportedColors() ) {
        if( const char* componentStr = znet::color_switch_converter::componentIdToStr( componentId ) ) {
            componentIds.push_back( componentStr );
        }
    }

    m_jsonBuilder << Class::ComponentIds << std::move( componentIds );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x40ThermostatMode& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    const auto modesCount = cc.getModesCount();
    JsonArray modes = Json::array();
    for( size_t i{ 0 }; i < modesCount; ++i ) {
        modes.push_back( cc.getMode( i ) );
    }

    m_jsonBuilder << Class::Modes << std::move( modes );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x42ThermostatOpState& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x43ThermostatSetpoint& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto setpoints = JSON_EMPTY_ARR;
    for( const auto& setpoint : cc.getDescription() ) {
        auto result = JSON( Class::Type << setpoint.type );

        if( cc.getVersion() >= 3 ) {
            result[ Class::MinValue ] = setpoint.minValue;
            result[ Class::MinScale ] = setpoint.minScale;
            result[ Class::MaxValue ] = setpoint.maxValue;
            result[ Class::MaxScale ] = setpoint.maxScale;
        }

        setpoints.push_back( std::move( result ) );
    }

    m_jsonBuilder << Class::Setpoints << std::move( setpoints );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x44ThermostatFanMode& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    JsonArray modes = Json::array();
    const auto modesCount = cc.getFanModesCount();
    for( size_t i{ 0 }; i < modesCount; ++i )
        modes.push_back( cc.getFanMode( i ) );

    m_jsonBuilder << Class::Modes << std::move( modes );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x45ThermostatFanState& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x47ThermostatSetback& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x4EScheduleEntryLock& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    m_jsonBuilder << Class::WeekDaySlots << cc.getWeekDaySlots() << Class::YearDaySlots << cc.getYearDaySlots()
                  << Class::DailyRepeatingSlots << cc.getDailyRepeatingSlots();
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x53Schedule& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto blocks = Json::array();

    cc.forEachBlock( [&cc, &blocks]( const uint8_t blockId,
                                     bool supportEnable,
                                     bool fallbackSupport,
                                     const std::vector< ScheduleStartTimeSupport >& startTimeSupport,
                                     bool overrideSupport,
                                     const std::vector< ScheduleDurationOverride >& supportedOverrideTypes ) {
        JsonObject block;

        block[ Class::BlockId ] = blockId;
        block[ Class::SupportEnable ] = supportEnable;
        block[ Class::FallbackSupport ] = fallbackSupport;
        block[ Class::OverrideSupport ] = overrideSupport;

        auto classes = Json::array();

        cc.forEachSupportedCc(
            blockId, [&cc, blockId, &classes]( const uint8_t ccId, const znet::SheduleSupportedCommandType type ) {
                JsonObject supportedClass;
                supportedClass[ Class::CCId ] = ccId;
                supportedClass[ Class::SupportedCcCommands ] =
                    znet::schedule_command_type_converter::typeToString( type );

                if( type == SheduleSupportedCommandType::All ) {
                    auto commands = Json::array();
                    cc.forEachCommand( blockId, ccId, [ccId, blockId, &commands]( const uint8_t command ) {
                        commands.push_back( command );
                    } );
                    supportedClass[ Class::Commands ] = std::move( commands );
                }
                classes.push_back( std::move( supportedClass ) );
            } );

        block[ Class::Classes ] = std::move( classes );

        auto& timeSupportTypes = block[ Class::StartTimeSupport ] = Json::array();

        for( const auto& type : startTimeSupport ) {
            timeSupportTypes.push_back( znet::schedule_start_time_support_converter::startTimeSupportToString( type ) );
        }

        auto& durationOverrideTypes = block[ Class::SupportedOverrideTypes ] = Json::array();

        for( const auto& type : supportedOverrideTypes ) {
            durationOverrideTypes.push_back( znet::schedule_override_type_converter::overrideTypeToString( type ) );
        }

        blocks.push_back( std::move( block ) );
    } );

    m_jsonBuilder << Class::SupportedScheduleIdsCount << cc.getNumberSupportedScheduleIds() << Class::ScheduleBlocks
                  << std::move( blocks );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x59AssociationGrpInfo& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    uint8_t groupsCount = cc.getAssociationGroupsCount();
    m_jsonBuilder << Class::AssociationGroupsCount << groupsCount;

    if( !groupsCount ) {
        return;
    }

    auto groups = Json::array();
    for( uint16_t i = 1; i <= groupsCount; ++i ) {
        if( !cc.hasGroup( i ) ) {
            continue;
        }

        JsonObject group;
        group[ Class::GroupId ] = i;

        const auto groupName = cc.getAssociationGroupName( i );
        if( !groupName ) {
            LOGGER_ERROR( "No group name for group " + std::to_string( i ) );
            continue;
        }
        group[ Class::GroupName ] = *groupName;

        const auto groupProfile = cc.getAssociationGroupProfile( i );
        if( !groupProfile ) {
            LOGGER_ERROR( "No group profile for group " + std::to_string( i ) );
            continue;
        }
        auto profileJson =
            JSON( Class::ProfileMsb << groupProfile->first << Class::ProfileLsb << groupProfile->second );
        auto profileName = znet::agi_profile_converter::profileToString( groupProfile->first, groupProfile->second );
        if( !profileName.empty() ) {
            profileJson[ Class::Name ] = std::move( profileName );
        }
        group[ Class::Profile ] = std::move( profileJson );

        const auto groupCommands = cc.getAssociationGroupCommands( i );
        if( !groupCommands ) {
            LOGGER_ERROR( "No group commands for group " + std::to_string( i ) );
            continue;
        }
        auto commands = Json::array();
        for( const auto& command : *groupCommands ) {
            JsonObject commandJson;
            commandJson[ Class::CCId ] = command.first;
            commandJson[ Class::CommandId ] = command.second;
            const auto* commandName = znet::agi_commands_converter::commandToString( command.first, command.second );
            if( commandName ) {
                commandJson[ Class::Name ] = commandName;
            }

            commands.push_back( std::move( commandJson ) );
        }

        group[ Class::Commands ] = std::move( commands );

        groups.push_back( std::move( group ) );
    }

    m_jsonBuilder << Class::AssociationGroups << std::move( groups );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x5DAntiTheft& cc ) {
    ( *this )( static_cast< const ZCcAntiTheftBase& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x60MultiChannel& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x62DoorLock& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    if( const auto capabilities = cc.getCapabilities() ) {
        auto jsonCapabilities = Json::object();

        jsonCapabilities[ Class::DoorSupported ] = capabilities->isDoorSupported;
        jsonCapabilities[ Class::BoltSupported ] = capabilities->isBoltSupported;
        jsonCapabilities[ Class::LatchSupported ] = capabilities->isLatchSupported;
        jsonCapabilities[ Class::BlockToBlockSupported ] = capabilities->isBlockToBlockSupported;
        jsonCapabilities[ Class::TwistAssistSupported ] = capabilities->isTwistAssistSupported;
        jsonCapabilities[ Class::HoldAndReleaseSupported ] = capabilities->isHoldAndReleaseSupported;
        jsonCapabilities[ Class::TimedOperationsSupported ] = capabilities->areTimeOperationsSupported;
        jsonCapabilities[ Class::AutoRelockSupported ] = capabilities->isAutoRelockSupported;

        auto& jsonDoorLockModes = jsonCapabilities[ Class::DoorLockModes ] = Json::array();
        for( const auto mode : capabilities->supportedModes ) {
            if( const char* strMode = znet::door_lock_converter::modeToString( mode ) ) {
                jsonDoorLockModes.push_back( strMode );
            }
            else {
                LOGGER_WARN( "Cannot recognize doorlock mode " + std::to_string( static_cast< uint8_t >( mode ) ) );
            }
        }

        auto& jsonInsideHandleIds = jsonCapabilities[ Class::InsideHandleIds ] = Json::array();
        znet::utils::forEachBit(
            &capabilities->supportedInsideDoorHandlesBitmask,
            sizeof( capabilities->supportedInsideDoorHandlesBitmask ),
            [&jsonInsideHandleIds]( uint8_t index ) { jsonInsideHandleIds.push_back( index + 1 ); } );

        auto& jsonOutsideHandleIds = jsonCapabilities[ Class::OutsideHandleIds ] = Json::array();
        znet::utils::forEachBit(
            &capabilities->supportedOutsideDoorHandlesBitmask,
            sizeof( capabilities->supportedOutsideDoorHandlesBitmask ),
            [&jsonOutsideHandleIds]( uint8_t index ) { jsonOutsideHandleIds.push_back( index + 1 ); } );

        m_jsonBuilder << Class::Capabilities << std::move( jsonCapabilities );
    }
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x66BarrierOperator& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    const auto& supportedSignals = cc.getSupportedSignals();
    auto supportedSignalJson = Json::array();
    for( const auto& signal : supportedSignals ) {
        if( const auto* signalName = znet::barrier_operator_signal_converter::signalToString(
                static_cast< BarrierOperatorSignal >( signal ) ) ) {
            supportedSignalJson.push_back( signalName );
        }
    }

    m_jsonBuilder << Class::Signals << std::move( supportedSignalJson );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x70Configuration& cc ) {
    using namespace znet::signed_configuration_parameter_converter;

    ( *this )( static_cast< const ZClass& >( cc ) );

    auto supportedParametersJson = Json::array();
    cc.forEachV3Parameter( [&supportedParametersJson]( const ZCc0x70Configuration::Parameter& parameter ) {
        if( const char* format = znet::configuration_format_converter::formatToString( parameter.m_format ) ) {
            JsonObject paramJson;
            paramJson[ Class::Id ] = parameter.m_id;
            paramJson[ Class::Format ] = format;
            paramJson[ Class::Size ] = parameter.m_size;
            paramJson[ Class::ReadOnly ] = parameter.m_readOnly;
            paramJson[ Class::MinValue ] = toUiValue( parameter.m_minValue, parameter.m_size, parameter.m_format );
            paramJson[ Class::MaxValue ] = toUiValue( parameter.m_maxValue, parameter.m_size, parameter.m_format );
            paramJson[ Class::DefaultValue ] =
                toUiValue( parameter.m_defaultValue, parameter.m_size, parameter.m_format );
            paramJson[ Class::Name ] = parameter.m_name;
            paramJson[ Class::Info ] = parameter.m_info;

            supportedParametersJson.push_back( std::move( paramJson ) );
        }
    } );

    m_jsonBuilder << Class::Parameters << std::move( supportedParametersJson );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x71Notification& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto notifications = Json::array();
    for( const auto& notificationInfo : cc.getSupportedNotifications() ) {
        if( const auto name = getNotificationName( notificationInfo.m_type ) ) {
            JsonObject notification;
            notification[ Class::Type ] = name;
            auto& eventTypes = notification[ Class::NotificationEventTypes ] = Json::array();
            auto& unknownEventTypes = notification[ Class::UnknownNotificationEventTypes ] = Json::array();
            for( const auto eventType : notificationInfo.m_events ) {
                auto eventData = getNotificationEventData( notificationInfo.m_type, eventType );
                if( eventData && cc.getVersion() >= eventData->m_version ) {
                    eventTypes.push_back( eventData->m_name );
                }
                else {
                    unknownEventTypes.push_back( eventType );
                }
            }

            notifications.push_back( std::move( notification ) );
        }
    }

    m_jsonBuilder << Class::Notifications << std::move( notifications ) << Class::IsNotificationV1Supported
                  << cc.isV1Supported();
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x72ManufacturerSpecific& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x80Battery& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x81Clock& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x84WakeUp& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    m_jsonBuilder << Node::WakeUpInterval << cc.getInterval();

    if( cc.hasCapabilities() ) {
        m_jsonBuilder << Class::MaxWakeUpInterval << cc.getMaxWakeUpInterval() << Class::MinWakeUpInterval
                      << cc.getMinWakeUpInterval() << Class::StepWakeUpInterval << cc.getStepWakeUpInterval();
    }
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x85Association& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    uint8_t groupsCount = cc.getAssociationGroupsCount();
    m_jsonBuilder << Class::AssociationGroupsCount << groupsCount;

    if( !groupsCount ) {
        return;
    }

    auto groups = Json::array();
    for( uint16_t i = 1; i <= groupsCount; ++i ) {
        JsonObject group;
        group[ Class::GroupId ] = i;

        group[ Class::GroupNodesMaxCount ] = cc.getGroupMaxNodesCount( i );

        const auto* groupNodes = cc.getGroupNodes( i );
        if( groupNodes ) {
            auto nodesJson = Json::array();
            for( auto nodeId : *groupNodes ) {
                nodesJson.push_back( nodeId );
            }
            group[ Class::GroupNodes ] = std::move( nodesJson );
        }
        groups.push_back( std::move( group ) );
    }

    m_jsonBuilder << Class::AssociationGroups << std::move( groups );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x8eMultiChannelAssociation& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    uint8_t groupsCount = cc.getAssociationGroupsCount();
    m_jsonBuilder << Class::AssociationGroupsCount << groupsCount;

    if( !groupsCount ) {
        return;
    }

    auto groups = Json::array();
    for( uint16_t i = 1; i <= groupsCount; ++i ) {
        JsonObject group;
        group[ Class::GroupId ] = i;

        group[ Class::GroupNodesMaxCount ] = cc.getGroupMaxNodesCount( i );

        const auto* groupInfo = cc.getGroupInfo( i );
        if( groupInfo ) {
            auto nodesJson = Json::array();
            for( const auto& nodeInfo : *groupInfo ) {
                JsonObject nodeInfoJson;
                nodeInfoJson[ Class::NodeId ] = nodeInfo.node_id;
                nodeInfoJson[ Class::EndpointId ] = nodeInfo.channel_id;

                nodesJson.push_back( std::move( nodeInfoJson ) );
            }
            group[ Class::GroupNodes ] = std::move( nodesJson );
        }
        groups.push_back( std::move( group ) );
    }

    m_jsonBuilder << Class::AssociationGroups << std::move( groups );
}

void ZwaveCmdClassSerializationVisitor::operator()( const ZCc0x9cAlarmSensor& cc ) {
    auto sensorTypes = Json::array();
    for( const auto type : cc.getSupportedTypes() ) {
        sensorTypes.push_back( znet::getExtendedAlarmSensorName( type ) );
    }

    m_jsonBuilder << Class::SensorTypes << std::move( sensorTypes );

    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x9dAlarmSilence& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x8ATime& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x8BTimeParameters& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x86Version& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    auto firmwareVersions = Json::array();

    for( uint8_t index = 0; index != cc.getFirmwaresCount(); ++index ) {
        firmwareVersions.push_back( cc.getFirmwareVersion( index ) );
    }

    m_jsonBuilder << VersionInfo::FirmwareVersions << std::move( firmwareVersions );

    addOptionalField( m_jsonBuilder, VersionInfo::LibraryType, cc.getLibraryType() );
    addOptionalField( m_jsonBuilder, VersionInfo::ZwaveProtocolVersion, cc.getZwaveProtocolVersion() );
    addOptionalField( m_jsonBuilder, VersionInfo::HardwareVersion, cc.getHardwareVersion() );

    addOptionalField( m_jsonBuilder, VersionInfo::HardwareVersion, cc.getHardwareVersion() );

    addOptionalField( m_jsonBuilder, VersionInfo::SdkVersion, cc.getSdkVersion() );

    addOptionalField(
        m_jsonBuilder, VersionInfo::ApplicationFrameworkApiVersion, cc.getApplicationFrameworkApiVersion() );
    addOptionalField(
        m_jsonBuilder, VersionInfo::ApplicationFrameworkApiBuildNumber, cc.getApplicationFrameworkApiBuildNumber() );

    addOptionalField( m_jsonBuilder, VersionInfo::HostInterfaceVersion, cc.getHostInterfaceVersion() );
    addOptionalField( m_jsonBuilder, VersionInfo::HostInterfaceBuildNumber, cc.getHostInterfaceBuildNumber() );

    addOptionalField( m_jsonBuilder, VersionInfo::ProtocolVersion, cc.getProtocolVersion() );
    addOptionalField( m_jsonBuilder, VersionInfo::ProtocolBuildNumber, cc.getProtocolBuildNumber() );

    addOptionalField( m_jsonBuilder, VersionInfo::ApplicationVersion, cc.getApplicationVersion() );
    addOptionalField( m_jsonBuilder, VersionInfo::ApplicationBuildNumber, cc.getApplicationBuildNumber() );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x87Indicator& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x5EZwavePlusInfo& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x79SoundSwitch& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    JsonArray toneInfos = Json::array();

    const uint8_t tonesNumber = cc.getTonesNumber();
    for( uint8_t toneId = 1; toneId <= tonesNumber; ++toneId ) {
        JsonObject toneInfoJson;
        toneInfoJson[ Sound::ToneId ] = toneId;
        toneInfoJson[ Sound::ToneName ] = cc.getToneName( toneId );
        toneInfoJson[ Sound::Duration ] = cc.getToneDuration( toneId );

        toneInfos.push_back( std::move( toneInfoJson ) );
    }

    m_jsonBuilder << Sound::Tones << std::move( toneInfos );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x7AFirmwareUpdate& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    m_jsonBuilder << Class::FirmwareIds << cc.getFirmwareIds();
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x7EAntiTheftUnlock& cc ) {
    ( *this )( static_cast< const ZCcAntiTheftBase& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCcAntiTheftBase& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );

    if( const auto& lockInfo = cc.getInitialLockInfo() ) {
        JsonObject lockInfoJson;
        lockInfoJson[ Class::Hint ] = lockInfo->getHexHint();
        lockInfoJson[ Class::IsRestricted ] = lockInfo->m_isRestricted;
        lockInfoJson[ Class::ManufacturerId ] = lockInfo->m_manufacturerId;
        if( lockInfo->m_zwaveAllianceLockingId ) {
            lockInfoJson[ Class::ZWaveAllianceLockingId ] = *lockInfo->m_zwaveAllianceLockingId;
        }

        m_jsonBuilder << Class::InitialLockInfo << std::move( lockInfoJson );
    }
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x98Security& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

void ZwaveCmdClassSerializationVisitor::operator()( const znet::ZCc0x9FSecurity2& cc ) {
    ( *this )( static_cast< const ZClass& >( cc ) );
}

Json ZwaveCmdClassSerializationVisitor::takeResult() {
    auto result = m_jsonBuilder.getJson();
    m_jsonBuilder = JsonBuilder();
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
