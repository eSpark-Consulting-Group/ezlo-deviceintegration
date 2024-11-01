#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/command_id.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

static constexpr uint8_t UnknownCommandId = 0xFF;

template < typename AttributeId >
struct CommandIdData {
    AttributeId commandId;
    const char* name;
};

static const CommandIdData< IasAceServerCommandId > iasAceServerCommandIdData[] = {
    { IasAceServerCommandId::ArmResponse, Resources::Fields::Command::Names::IasAce::ArmResponse },
    { IasAceServerCommandId::GetZoneIDMapResponse, Resources::Fields::Command::Names::IasAce::GetZoneIDMapResponse },
    { IasAceServerCommandId::GetZoneInformationResponse,
      Resources::Fields::Command::Names::IasAce::GetZoneInformationResponse },
    { IasAceServerCommandId::ZoneStatusChanged, Resources::Fields::Command::Names::IasAce::ZoneStatusChanged },
    { IasAceServerCommandId::PanelStatusChanged, Resources::Fields::Command::Names::IasAce::PanelStatusChanged },
    { IasAceServerCommandId::GetPanelStatusResponse,
      Resources::Fields::Command::Names::IasAce::GetPanelStatusResponse },
    { IasAceServerCommandId::SetBypassedZoneList, Resources::Fields::Command::Names::IasAce::SetBypassedZoneList },
    { IasAceServerCommandId::BypassResponse, Resources::Fields::Command::Names::IasAce::BypassResponse },
    { IasAceServerCommandId::GetZoneStatusResponse, Resources::Fields::Command::Names::IasAce::GetZoneStatusResponse }
};

static const CommandIdData< IasAceClientCommandId > iasAceClientCommandIdData[] = {
    { IasAceClientCommandId::Arm, Resources::Fields::Command::Names::IasAce::Arm },
    { IasAceClientCommandId::Bypass, Resources::Fields::Command::Names::IasAce::Bypass },
    { IasAceClientCommandId::Emergency, Resources::Fields::Command::Names::IasAce::Emergency },
    { IasAceClientCommandId::Fire, Resources::Fields::Command::Names::IasAce::Fire },
    { IasAceClientCommandId::Panic, Resources::Fields::Command::Names::IasAce::Panic },
    { IasAceClientCommandId::GetZoneIDMap, Resources::Fields::Command::Names::IasAce::GetZoneIDMap },
    { IasAceClientCommandId::GetZoneInformation, Resources::Fields::Command::Names::IasAce::GetZoneInformation },
    { IasAceClientCommandId::GetPanelStatus, Resources::Fields::Command::Names::IasAce::GetPanelStatus },
    { IasAceClientCommandId::GetBypassedZoneList, Resources::Fields::Command::Names::IasAce::GetBypassedZoneList },
    { IasAceClientCommandId::GetZoneStatus, Resources::Fields::Command::Names::IasAce::GetZoneStatus }
};

static const CommandIdData< PollControlServerCommandId > pollControlServerCommandIdData[] = {
    { PollControlServerCommandId::CheckIn, Resources::Fields::Command::Names::PollControl::CheckIn }
};

static const CommandIdData< PollControlClientCommandId > pollControlClientCommandIdData[] = {
    { PollControlClientCommandId::CheckInResponse, Resources::Fields::Command::Names::PollControl::CheckInResponse },
    { PollControlClientCommandId::FastPollStop, Resources::Fields::Command::Names::PollControl::FastPollStop },
    { PollControlClientCommandId::SetLongPollInterval,
      Resources::Fields::Command::Names::PollControl::SetLongPollInterval },
    { PollControlClientCommandId::SetShortPollInterval,
      Resources::Fields::Command::Names::PollControl::SetShortPollInterval }
};

static const CommandIdData< OnOffClientCommandId > onOffClientCommandIdData[] = {
    { OnOffClientCommandId::Off, Resources::Fields::Command::Names::OnOff::Off },
    { OnOffClientCommandId::On, Resources::Fields::Command::Names::OnOff::On },
    { OnOffClientCommandId::Toggle, Resources::Fields::Command::Names::OnOff::Toggle },
    { OnOffClientCommandId::OffWithEffect, Resources::Fields::Command::Names::OnOff::OffWithEffect },
    { OnOffClientCommandId::OnWithRecallGlobalScene,
      Resources::Fields::Command::Names::OnOff::OnWithRecallGlobalScene },
    { OnOffClientCommandId::OnWithTimedOff, Resources::Fields::Command::Names::OnOff::OnWithTimedOff }
};

static const CommandIdData< LevelControlClientCommandId > levelControlClientCommandIdData[] = {
    { LevelControlClientCommandId::MoveToLevel, Resources::Fields::Command::Names::LevelControl::MoveToLevel },
    { LevelControlClientCommandId::Move, Resources::Fields::Command::Names::LevelControl::Move },
    { LevelControlClientCommandId::Step, Resources::Fields::Command::Names::LevelControl::Step },
    { LevelControlClientCommandId::Stop, Resources::Fields::Command::Names::LevelControl::Stop },
    { LevelControlClientCommandId::MoveToLevelWithOnOff,
      Resources::Fields::Command::Names::LevelControl::MoveToLevelWithOnOff },
    { LevelControlClientCommandId::MoveWithOnOff, Resources::Fields::Command::Names::LevelControl::MoveWithOnOff },
    { LevelControlClientCommandId::StepWithOnOff, Resources::Fields::Command::Names::LevelControl::StepWithOnOff },
    { LevelControlClientCommandId::StopWithOnOff, Resources::Fields::Command::Names::LevelControl::StopWithOnOff },
    { LevelControlClientCommandId::MoveToClosestFrequency,
      Resources::Fields::Command::Names::LevelControl::MoveToClosestFrequency }
};

static const CommandIdData< ColorControlClientCommandId > colorControlClientCommandIdData[] = {
    { ColorControlClientCommandId::MoveToHue, Resources::Fields::Command::Names::ColorControl::MoveToHue },
    { ColorControlClientCommandId::MoveHue, Resources::Fields::Command::Names::ColorControl::MoveHue },
    { ColorControlClientCommandId::StepHue, Resources::Fields::Command::Names::ColorControl::StepHue },
    { ColorControlClientCommandId::MoveToSaturation,
      Resources::Fields::Command::Names::ColorControl::MoveToSaturation },
    { ColorControlClientCommandId::MoveSaturation, Resources::Fields::Command::Names::ColorControl::MoveSaturation },
    { ColorControlClientCommandId::StepSaturation, Resources::Fields::Command::Names::ColorControl::StepSaturation },
    { ColorControlClientCommandId::MoveToHueAndSaturation,
      Resources::Fields::Command::Names::ColorControl::MoveToHueAndSaturation },
    { ColorControlClientCommandId::MoveToColor, Resources::Fields::Command::Names::ColorControl::MoveToColor },
    { ColorControlClientCommandId::MoveColor, Resources::Fields::Command::Names::ColorControl::MoveColor },
    { ColorControlClientCommandId::StepColor, Resources::Fields::Command::Names::ColorControl::StepColor },
    { ColorControlClientCommandId::MoveToColorTemperature,
      Resources::Fields::Command::Names::ColorControl::MoveToColorTemperature },
    { ColorControlClientCommandId::EnhancedMoveToHue,
      Resources::Fields::Command::Names::ColorControl::EnhancedMoveToHue },
    { ColorControlClientCommandId::EnhancedMoveHue, Resources::Fields::Command::Names::ColorControl::EnhancedMoveHue },
    { ColorControlClientCommandId::EnhancedStepHue, Resources::Fields::Command::Names::ColorControl::EnhancedStepHue },
    { ColorControlClientCommandId::EnhancedMoveToHueAndSaturation,
      Resources::Fields::Command::Names::ColorControl::EnhancedMoveToHueAndSaturation },
    { ColorControlClientCommandId::ColorLoopSet, Resources::Fields::Command::Names::ColorControl::ColorLoopSet },
    { ColorControlClientCommandId::StopMoveStep, Resources::Fields::Command::Names::ColorControl::StopMoveStep },
    { ColorControlClientCommandId::MoveColorTemperature,
      Resources::Fields::Command::Names::ColorControl::MoveColorTemperature },
    { ColorControlClientCommandId::StepColorTemperature,
      Resources::Fields::Command::Names::ColorControl::StepColorTemperature }
};

static const CommandIdData< DoorLockServerCommandId > doorLockServerCommandIdData[] = {
    { DoorLockServerCommandId::LockDoorResponse, Resources::Fields::Command::Names::DoorLock::LockDoorResponse },
    { DoorLockServerCommandId::UnlockDoorResponse, Resources::Fields::Command::Names::DoorLock::UnlockDoorResponse },
    { DoorLockServerCommandId::ToggleResponse, Resources::Fields::Command::Names::DoorLock::ToggleResponse },
    { DoorLockServerCommandId::UnlockWithTimeoutResponse,
      Resources::Fields::Command::Names::DoorLock::UnlockWithTimeoutResponse },
    { DoorLockServerCommandId::GetLogRecordResponse,
      Resources::Fields::Command::Names::DoorLock::GetLogRecordResponse },
    { DoorLockServerCommandId::SetPinCodeResponse, Resources::Fields::Command::Names::DoorLock::SetPinCodeResponse },
    { DoorLockServerCommandId::GetPinCodeResponse, Resources::Fields::Command::Names::DoorLock::GetPinCodeResponse },
    { DoorLockServerCommandId::ClearPinCodeResponse,
      Resources::Fields::Command::Names::DoorLock::ClearPinCodeResponse },
    { DoorLockServerCommandId::ClearAllPinCodesResponse,
      Resources::Fields::Command::Names::DoorLock::ClearAllPinCodesResponse },
    { DoorLockServerCommandId::SetUserStatusResponse,
      Resources::Fields::Command::Names::DoorLock::SetUserStatusResponse },
    { DoorLockServerCommandId::GetUserStatusResponse,
      Resources::Fields::Command::Names::DoorLock::GetUserStatusResponse },
    { DoorLockServerCommandId::SetWeekdayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::SetWeekdayScheduleResponse },
    { DoorLockServerCommandId::GetWeekdayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::GetWeekdayScheduleResponse },
    { DoorLockServerCommandId::ClearWeekdayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::ClearWeekdayScheduleResponse },
    { DoorLockServerCommandId::SetYearDayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::SetYearDayScheduleResponse },
    { DoorLockServerCommandId::GetYearDayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::GetYearDayScheduleResponse },
    { DoorLockServerCommandId::ClearYearDayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::ClearYearDayScheduleResponse },
    { DoorLockServerCommandId::SetHolidayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::SetHolidayScheduleResponse },
    { DoorLockServerCommandId::GetHolidayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::GetHolidayScheduleResponse },
    { DoorLockServerCommandId::ClearHolidayScheduleResponse,
      Resources::Fields::Command::Names::DoorLock::ClearHolidayScheduleResponse },
    { DoorLockServerCommandId::SetUserTypeResponse, Resources::Fields::Command::Names::DoorLock::SetUserTypeResponse },
    { DoorLockServerCommandId::GetUserTypeResponse, Resources::Fields::Command::Names::DoorLock::GetUserTypeResponse },
    { DoorLockServerCommandId::SetRfidCodeResponse, Resources::Fields::Command::Names::DoorLock::SetRfidCodeResponse },
    { DoorLockServerCommandId::GetRfidCodeResponse, Resources::Fields::Command::Names::DoorLock::GetRfidCodeResponse },
    { DoorLockServerCommandId::ClearRfidCodeResponse,
      Resources::Fields::Command::Names::DoorLock::ClearRfidCodeResponse },
    { DoorLockServerCommandId::ClearAllRfidCodesResponse,
      Resources::Fields::Command::Names::DoorLock::ClearAllRfidCodesResponse },
    { DoorLockServerCommandId::OperatingEventNotification,
      Resources::Fields::Command::Names::DoorLock::OperatingEventNotification },
    { DoorLockServerCommandId::ProgrammingEventNotification,
      Resources::Fields::Command::Names::DoorLock::ProgrammingEventNotification }
};

static const CommandIdData< DoorLockClientCommandId > doorLockClientCommandIdData[] = {
    { DoorLockClientCommandId::LockDoor, Resources::Fields::Command::Names::DoorLock::LockDoor },
    { DoorLockClientCommandId::UnlockDoor, Resources::Fields::Command::Names::DoorLock::UnlockDoor },
    { DoorLockClientCommandId::Toggle, Resources::Fields::Command::Names::DoorLock::Toggle },
    { DoorLockClientCommandId::UnlockWithTimeout, Resources::Fields::Command::Names::DoorLock::UnlockWithTimeout },
    { DoorLockClientCommandId::GetLogRecord, Resources::Fields::Command::Names::DoorLock::GetLogRecord },
    { DoorLockClientCommandId::SetPinCode, Resources::Fields::Command::Names::DoorLock::SetPinCode },
    { DoorLockClientCommandId::GetPinCode, Resources::Fields::Command::Names::DoorLock::GetPinCode },
    { DoorLockClientCommandId::ClearPinCode, Resources::Fields::Command::Names::DoorLock::ClearPinCode },
    { DoorLockClientCommandId::ClearAllPinCodes, Resources::Fields::Command::Names::DoorLock::ClearAllPinCodes },
    { DoorLockClientCommandId::SetUserStatus, Resources::Fields::Command::Names::DoorLock::SetUserStatus },
    { DoorLockClientCommandId::GetUserStatus, Resources::Fields::Command::Names::DoorLock::GetUserStatus },
    { DoorLockClientCommandId::SetWeekdaySchedule, Resources::Fields::Command::Names::DoorLock::SetWeekdaySchedule },
    { DoorLockClientCommandId::GetWeekdaySchedule, Resources::Fields::Command::Names::DoorLock::GetWeekdaySchedule },
    { DoorLockClientCommandId::ClearWeekdaySchedule,
      Resources::Fields::Command::Names::DoorLock::ClearWeekdaySchedule },
    { DoorLockClientCommandId::SetYearDaySchedule, Resources::Fields::Command::Names::DoorLock::SetYearDaySchedule },
    { DoorLockClientCommandId::GetYearDaySchedule, Resources::Fields::Command::Names::DoorLock::GetYearDaySchedule },
    { DoorLockClientCommandId::ClearYearDaySchedule,
      Resources::Fields::Command::Names::DoorLock::ClearYearDaySchedule },
    { DoorLockClientCommandId::SetHolidaySchedule, Resources::Fields::Command::Names::DoorLock::SetHolidaySchedule },
    { DoorLockClientCommandId::GetHolidaySchedule, Resources::Fields::Command::Names::DoorLock::GetHolidaySchedule },
    { DoorLockClientCommandId::ClearHolidaySchedule,
      Resources::Fields::Command::Names::DoorLock::ClearHolidaySchedule },
    { DoorLockClientCommandId::SetUserType, Resources::Fields::Command::Names::DoorLock::SetUserType },
    { DoorLockClientCommandId::GetUserType, Resources::Fields::Command::Names::DoorLock::GetUserType },
    { DoorLockClientCommandId::SetRfidCode, Resources::Fields::Command::Names::DoorLock::SetRfidCode },
    { DoorLockClientCommandId::GetRfidCode, Resources::Fields::Command::Names::DoorLock::GetRfidCode },
    { DoorLockClientCommandId::ClearRfidCode, Resources::Fields::Command::Names::DoorLock::ClearRfidCode },
    { DoorLockClientCommandId::ClearAllRfidCodes, Resources::Fields::Command::Names::DoorLock::ClearAllRfidCodes }
};

enum class CommandHoler { Server, Client, Unknown };

template < typename AttributeId, size_t SIZE >
const char* toStringInternal( AttributeId attributeId, const CommandIdData< AttributeId > ( &datas )[ SIZE ] ) {
    for( const auto& data : datas ) {
        if( data.commandId == attributeId ) {
            return data.name;
        }
    }

    return Resources::Fields::Command::Names::Unknown;
}

template < typename AttributeId, size_t SIZE >
uint8_t fromStringInternal( const std::string& attributeName, const CommandIdData< AttributeId > ( &datas )[ SIZE ] ) {
    for( const auto& data : datas ) {
        if( attributeName == data.name ) {
            return static_cast< uint8_t >( data.commandId );
        }
    }

    assert( !"Unknown command name" );

    return UnknownCommandId;
}

CommandHoler getCommandHolerInternal( ZigbeeClusterBase::ClusterDataHolder clusterType, CommandType commandType ) {
    switch( commandType ) {
    case CommandType::Generated:
        switch( clusterType ) {
        case ZigbeeClusterBase::ClusterDataHolder::Server:
            return CommandHoler::Server;
        case ZigbeeClusterBase::ClusterDataHolder::Client:
            return CommandHoler::Client;
        default:
            return CommandHoler::Unknown;
        }
        break;

    case CommandType::Received:
        switch( clusterType ) {
        case ZigbeeClusterBase::ClusterDataHolder::Server:
            return CommandHoler::Client;
        case ZigbeeClusterBase::ClusterDataHolder::Client:
            return CommandHoler::Server;
        default:
            return CommandHoler::Unknown;
        }
        break;

    default:
        return CommandHoler::Unknown;
    }
}

const char* toString( ClusterId clusterId,
                      ZigbeeClusterBase::ClusterDataHolder clusterType,
                      CommandType commandType,
                      uint16_t commandId ) {
    auto commandHolder = getCommandHolerInternal( clusterType, commandType );
    if( commandHolder == CommandHoler::Unknown ) {
        return Resources::Fields::Command::Names::Unknown;
    }

    switch( clusterId ) {
    case ClusterId::IASAce:
        if( commandHolder == CommandHoler::Server ) {
            return toStringInternal( static_cast< IasAceServerCommandId >( commandId ), iasAceServerCommandIdData );
        }
        else {
            return toStringInternal( static_cast< IasAceClientCommandId >( commandId ), iasAceClientCommandIdData );
        }
        break;

    case ClusterId::PollControl:
        if( commandHolder == CommandHoler::Server ) {
            return toStringInternal( static_cast< PollControlServerCommandId >( commandId ),
                                     pollControlServerCommandIdData );
        }
        else {
            return toStringInternal( static_cast< PollControlClientCommandId >( commandId ),
                                     pollControlClientCommandIdData );
        }
        break;

    case ClusterId::OnOff:
        if( commandHolder == CommandHoler::Client ) {
            return toStringInternal( static_cast< OnOffClientCommandId >( commandId ), onOffClientCommandIdData );
        }
        break;

    case ClusterId::LevelControl:
        if( commandHolder == CommandHoler::Client ) {
            return toStringInternal( static_cast< LevelControlClientCommandId >( commandId ),
                                     levelControlClientCommandIdData );
        }
        break;

    case ClusterId::ColorControl:
        if( commandHolder == CommandHoler::Client ) {
            return toStringInternal( static_cast< ColorControlClientCommandId >( commandId ),
                                     colorControlClientCommandIdData );
        }
        break;

    case ClusterId::DoorLock:
        if( commandHolder == CommandHoler::Server ) {
            return toStringInternal( static_cast< DoorLockServerCommandId >( commandId ), doorLockServerCommandIdData );
        }
        else {
            return toStringInternal( static_cast< DoorLockClientCommandId >( commandId ), doorLockClientCommandIdData );
        }
        break;
    }

    return Resources::Fields::Command::Names::Unknown;
}

uint8_t commandIdFromString( ClusterId clusterId,
                             CommandType commandType,
                             ZigbeeClusterBase::ClusterDataHolder clusterType,
                             const std::string& commandName ) {
    auto commandHolder = getCommandHolerInternal( clusterType, commandType );
    if( commandHolder == CommandHoler::Unknown ) {
        assert( !"Unknown command holder" );
        return UnknownCommandId;
    }

    switch( clusterId ) {
    case ClusterId::IASAce:
        if( commandHolder == CommandHoler::Server ) {
            return fromStringInternal( commandName, iasAceServerCommandIdData );
        }
        else {
            return fromStringInternal( commandName, iasAceClientCommandIdData );
        }
        break;

    case ClusterId::PollControl:
        if( commandHolder == CommandHoler::Server ) {
            return fromStringInternal( commandName, pollControlServerCommandIdData );
        }
        else {
            return fromStringInternal( commandName, pollControlClientCommandIdData );
        }
        break;

    case ClusterId::OnOff:
        if( commandHolder == CommandHoler::Client ) {
            return fromStringInternal( commandName, onOffClientCommandIdData );
        }
        break;

    case ClusterId::LevelControl:
        if( commandHolder == CommandHoler::Client ) {
            return fromStringInternal( commandName, levelControlClientCommandIdData );
        }
        break;

    case ClusterId::ColorControl:
        if( commandHolder == CommandHoler::Client ) {
            return fromStringInternal( commandName, colorControlClientCommandIdData );
        }
        break;

    case ClusterId::DoorLock:
        if( commandHolder == CommandHoler::Server ) {
            return fromStringInternal( commandName, doorLockServerCommandIdData );
        }
        else {
            return fromStringInternal( commandName, doorLockClientCommandIdData );
        }
        break;
    }

    assert( !"Unknown command name" );
    return UnknownCommandId;
}

}  // namespace Zigbee
}  // namespace HubPlatform
