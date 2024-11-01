#pragma once

#include "zigbee/model/zigbee_cluster.h"

#include <stdint-gcc.h>
#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class ClusterId;
enum class CommandType;

enum class IasAceServerCommandId {
    ArmResponse = 0x00,
    GetZoneIDMapResponse = 0x01,
    GetZoneInformationResponse = 0x02,
    ZoneStatusChanged = 0x03,
    PanelStatusChanged = 0x04,
    GetPanelStatusResponse = 0x05,
    SetBypassedZoneList = 0x06,
    BypassResponse = 0x07,
    GetZoneStatusResponse = 0x08
};

enum class IasAceClientCommandId {
    Arm = 0x00,
    Bypass = 0x01,
    Emergency = 0x02,
    Fire = 0x03,
    Panic = 0x04,
    GetZoneIDMap = 0x05,
    GetZoneInformation = 0x06,
    GetPanelStatus = 0x07,
    GetBypassedZoneList = 0x08,
    GetZoneStatus = 0x09
};

enum class PollControlServerCommandId { CheckIn = 0x00 };

enum class PollControlClientCommandId {
    CheckInResponse = 0x00,
    FastPollStop = 0x01,
    SetLongPollInterval = 0x02,
    SetShortPollInterval = 0x03
};

enum class OnOffClientCommandId {
    Off = 0x00,
    On = 0x01,
    Toggle = 0x02,
    OffWithEffect = 0x40,
    OnWithRecallGlobalScene = 0x41,
    OnWithTimedOff = 0x42
};

enum class LevelControlClientCommandId {
    MoveToLevel = 0x00,
    Move = 0x01,
    Step = 0x02,
    Stop = 0x03,
    MoveToLevelWithOnOff = 0x04,
    MoveWithOnOff = 0x05,
    StepWithOnOff = 0x06,
    StopWithOnOff = 0x07,
    MoveToClosestFrequency = 0x08
};

enum class ColorControlClientCommandId {
    MoveToHue = 0x00,
    MoveHue = 0x01,
    StepHue = 0x02,
    MoveToSaturation = 0x03,
    MoveSaturation = 0x04,
    StepSaturation = 0x05,
    MoveToHueAndSaturation = 0x06,
    MoveToColor = 0x07,
    MoveColor = 0x08,
    StepColor = 0x09,
    MoveToColorTemperature = 0x0A,
    EnhancedMoveToHue = 0x40,
    EnhancedMoveHue = 0x41,
    EnhancedStepHue = 0x42,
    EnhancedMoveToHueAndSaturation = 0x43,
    ColorLoopSet = 0x44,
    StopMoveStep = 0x47,
    MoveColorTemperature = 0x4B,
    StepColorTemperature = 0x4C
};

enum class DoorLockServerCommandId {
    LockDoorResponse = 0x00,
    UnlockDoorResponse = 0x01,
    ToggleResponse = 0x02,
    UnlockWithTimeoutResponse = 0x03,
    GetLogRecordResponse = 0x04,
    SetPinCodeResponse = 0x05,
    GetPinCodeResponse = 0x06,
    ClearPinCodeResponse = 0x07,
    ClearAllPinCodesResponse = 0x08,
    SetUserStatusResponse = 0x09,
    GetUserStatusResponse = 0x0A,
    SetWeekdayScheduleResponse = 0x0B,
    GetWeekdayScheduleResponse = 0x0C,
    ClearWeekdayScheduleResponse = 0x0D,
    SetYearDayScheduleResponse = 0x0E,
    GetYearDayScheduleResponse = 0x0F,
    ClearYearDayScheduleResponse = 0x10,
    SetHolidayScheduleResponse = 0x11,
    GetHolidayScheduleResponse = 0x12,
    ClearHolidayScheduleResponse = 0x13,
    SetUserTypeResponse = 0x14,
    GetUserTypeResponse = 0x15,
    SetRfidCodeResponse = 0x16,
    GetRfidCodeResponse = 0x17,
    ClearRfidCodeResponse = 0x18,
    ClearAllRfidCodesResponse = 0x19,
    OperatingEventNotification = 0x20,
    ProgrammingEventNotification = 0x21
};

enum class DoorLockClientCommandId {
    LockDoor = 0x00,
    UnlockDoor = 0x01,
    Toggle = 0x02,
    UnlockWithTimeout = 0x03,
    GetLogRecord = 0x04,
    SetPinCode = 0x05,
    GetPinCode = 0x06,
    ClearPinCode = 0x07,
    ClearAllPinCodes = 0x08,
    SetUserStatus = 0x09,
    GetUserStatus = 0x0A,
    SetWeekdaySchedule = 0x0B,
    GetWeekdaySchedule = 0x0C,
    ClearWeekdaySchedule = 0x0D,
    SetYearDaySchedule = 0x0E,
    GetYearDaySchedule = 0x0F,
    ClearYearDaySchedule = 0x10,
    SetHolidaySchedule = 0x11,
    GetHolidaySchedule = 0x12,
    ClearHolidaySchedule = 0x13,
    SetUserType = 0x14,
    GetUserType = 0x15,
    SetRfidCode = 0x16,
    GetRfidCode = 0x17,
    ClearRfidCode = 0x18,
    ClearAllRfidCodes = 0x19
};

const char* toString( ClusterId clusterId,
                      ZigbeeClusterBase::ClusterDataHolder clusterType,
                      CommandType commandType,
                      uint16_t commandId );

uint8_t commandIdFromString( ClusterId clusterId,
                             CommandType commandType,
                             ZigbeeClusterBase::ClusterDataHolder clusterType,
                             const std::string& attributeName );

}  // namespace Zigbee
}  // namespace HubPlatform
