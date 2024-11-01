#include "cored/house_modes/resources/house_mode_resources.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace Resources {

namespace DBNames {

const char* const HouseModesDB = HaHouseModesDBName;
const char* const HouseModesProtectButtonsDB = HaHouseModesProtectButtonsDBName;
const char* const HouseModesCamerasDB = HaHouseModesCamerasDBName;
const char* const HouseModesProtectDevicesDB = HaHouseModesProtectDevicesDBName;
const char* const HouseModesSecurityThreatsDB = HaHouseModesSecurityThreatsDBName;
const char* const HouseModesCommandTimestampsDBName = HaHouseModesCommandTimestampsDBName;

}  // namespace DBNames

namespace HouseModeFields {

const char* const IdFieldName = db::ID_FIELD_NAME.c_str();
const char* const ModeNameFieldName = "name";
const char* const DescriptionFieldName = "description";
const char* const SwitchToDelayFieldName = "switchToDelay";
const char* const AlarmDelayFieldName = "alarmDelay";
const char* const DisarmedDevicesFieldName = "disarmedDevices";
const char* const DisarmedDefaultFieldName = "disarmedDefault";
const char* const ArmedCategoriesFieldName = "armedCategories";
const char* const CategoryFieldName = "category";
const char* const SubcategoriesFieldName = "subcategories";
const char* const NotificationsFieldName = "notifications";
const char* const NotificationsDefaultFieldName = "notificationsDefault";
const char* const AlarmsOffDevicesFieldName = "alarmsOffDevices";
const char* const AlarmCategoriesFieldName = "alarmCategories";
const char* const CamerasOffDevicesFieldName = "camerasOffDevices";
const char* const BypassDevicesFieldName = "bypassDevices";
const char* const IgnoreSwitchDelay = "ignoreSwitchDelay";
const char* const ProtectFieldName = "protect";
const char* const ArmedFieldName = "armed";

}  // namespace HouseModeFields

namespace HouseModeCommandFields {

const char* const CommandFieldName = "command";
const char* const ModeIdFieldName = "modeId";
const char* const TimestampFieldName = "timestamp";

}  // namespace HouseModeCommandFields

namespace BroadcastFields {
const char* const ModeIdFieldName = "modeId";
const char* const AlarmsOffDeviceFieldName = "alarmsOffDevice";
const char* const DisarmedDeviceFieldName = "disarmedDevice";
const char* const CamerasOffDeviceFieldName = "camerasOffDevice";
const char* const DeviceIdsFieldName = "deviceIds";
const char* const DeviceIdFieldName = "deviceId";
const char* const DevicePhaseFieldName = "phase";
const char* const DeviceStatusFieldName = "status";
const char* const DeviceTimestampFieldName = "timestamp";
const char* const DevicePendingDelay = "pendingDelay";
const char* const Silent = "silent";
const char* const Chime = "chime";
const char* const SoundType = "soundType";

}  // namespace BroadcastFields

namespace ScriptEvents {

namespace Name {

const char* const HouseModeSwitched = "house_mode_switched";
const char* const HouseModeAlarmed = "house_mode_alarmed";
const char* const HouseModeExitTimeStarted = "exit_time_started";
const char* const HouseModeExitTimeFinished = "exit_time_finished";
const char* const HouseModeExitTimeRunsOut = "exit_time_runs_out";
const char* const HouseModeEntryDelayStarted = "entry_delay_started";
const char* const HouseModeEntryDelayFinished = "entry_delay_finished";
const char* const HouseModeLocalAlarmsTurnedOff = "house_mode_local_alarms_turned_off";

}  // namespace Name

}  // namespace ScriptEvents

namespace ProtectService {
const char* const Police = "police";
const char* const Medical = "medical";
const char* const Fire = "fire";
}  // namespace ProtectService

namespace Items {
const char* const MakeRecording = "make_recording";
const char* const SecurityThreat = "security_threat";
const char* const ProtectAlarm = "protect_alarm";
}  // namespace Items

namespace AlarmPhase {
const char* const Phase = "phase";
const char* const SwitchTo = "switchTo";
const char* const Status = "status";
}  // namespace AlarmPhase

namespace ItemValues {
const char* const AlarmActive = "alarm_active";
const char* const AlarmActiveSilent = "alarm_active_silent";
}  // namespace ItemValues

namespace SosButtonEvents {
const char* const ProtectSosButtonAdded = "protect_sos_button_added";
const char* const ProtectSosButtonUpdated = "protect_sos_button_updated";
const char* const ProtectSosButtonRemoved = "protect_sos_button_removed";
}  // namespace SosButtonEvents

namespace WalkTestEvents {
const char* const SystemTestStarted = "system_test_started";
const char* const SystemTestExtended = "system_test_extended";
const char* const SystemTestFinished = "system_test_finished";
const char* const SystemTerminationProgress = "system_test_termination_progress";
}  // namespace WalkTestEvents

}  // namespace Resources
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
