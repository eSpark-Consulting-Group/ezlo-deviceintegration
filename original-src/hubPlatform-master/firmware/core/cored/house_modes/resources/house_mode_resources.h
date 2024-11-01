#pragma once

#include <map>
#include <string>
#include "database_client/db_tables_names.h"
#include <chrono>

namespace HubPlatform {
namespace CoreDHouseModes {
namespace Resources {

static const std::chrono::seconds ExitTimeRunsOut( 10 );

enum class EntryDelayStatus { DONE, CANCELED, SKIPPED };

namespace DBNames {

extern const char* const HouseModesDB;
extern const char* const HouseModesProtectButtonsDB;
extern const char* const HouseModesCamerasDB;
extern const char* const HouseModesProtectDevicesDB;
extern const char* const HouseModesSecurityThreatsDB;
extern const char* const HouseModesCommandTimestampsDBName;

}  // namespace DBNames

namespace HouseModeFields {

extern const char* const IdFieldName;
extern const char* const ModeNameFieldName;
extern const char* const DescriptionFieldName;
extern const char* const SwitchToDelayFieldName;
extern const char* const AlarmDelayFieldName;
extern const char* const DisarmedDefaultFieldName;
extern const char* const DisarmedDevicesFieldName;
extern const char* const ArmedCategoriesFieldName;
extern const char* const CategoryFieldName;
extern const char* const SubcategoriesFieldName;
extern const char* const NotificationsFieldName;
extern const char* const NotificationsDefaultFieldName;
extern const char* const AlarmsOffDevicesFieldName;
extern const char* const AlarmCategoriesFieldName;
extern const char* const CamerasOffDevicesFieldName;
extern const char* const BypassDevicesFieldName;
extern const char* const IgnoreSwitchDelay;
extern const char* const ProtectFieldName;
extern const char* const ArmedFieldName;

}  // namespace HouseModeFields

namespace HouseModeCommandFields {

extern const char* const CommandFieldName;
extern const char* const ModeIdFieldName;
extern const char* const TimestampFieldName;

}  // namespace HouseModeCommandFields

namespace ScriptEvents {

namespace Name {

extern const char* const HouseModeSwitched;
extern const char* const HouseModeAlarmed;
extern const char* const HouseModeExitTimeStarted;
extern const char* const HouseModeExitTimeFinished;
extern const char* const HouseModeExitTimeRunsOut;
extern const char* const HouseModeEntryDelayStarted;
extern const char* const HouseModeEntryDelayFinished;
extern const char* const HouseModeLocalAlarmsTurnedOff;

}  // namespace Name

}  // namespace ScriptEvents

namespace ProtectService {
extern const char* const Police;
extern const char* const Medical;
extern const char* const Fire;
}  // namespace ProtectService

namespace BroadcastFields {
extern const char* const ModeIdFieldName;
extern const char* const AlarmsOffDeviceFieldName;
extern const char* const DisarmedDeviceFieldName;
extern const char* const CamerasOffDeviceFieldName;
extern const char* const DeviceIdsFieldName;
extern const char* const DeviceIdFieldName;
extern const char* const DeviceTimestampFieldName;
extern const char* const DevicePhaseFieldName;
extern const char* const DeviceStatusFieldName;
extern const char* const DevicePendingDelay;
extern const char* const Silent;
extern const char* const Chime;
extern const char* const SoundType;
}  // namespace BroadcastFields

namespace Items {
extern const char* const MakeRecording;
extern const char* const SecurityThreat;
extern const char* const ProtectAlarm;
}  // namespace Items

namespace AlarmPhase {
extern const char* const Phase;
extern const char* const SwitchTo;
extern const char* const Status;
}  // namespace AlarmPhase

namespace ItemValues {
extern const char* const AlarmActive;
extern const char* const AlarmActiveSilent;
}  // namespace ItemValues

namespace SosButtonEvents {
extern const char* const ProtectSosButtonAdded;
extern const char* const ProtectSosButtonUpdated;
extern const char* const ProtectSosButtonRemoved;
}  // namespace SosButtonEvents

namespace WalkTestEvents {
extern const char* const SystemTestStarted;
extern const char* const SystemTestExtended;
extern const char* const SystemTestFinished;
extern const char* const SystemTerminationProgress;
}  // namespace WalkTestEvents

}  // namespace Resources
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
