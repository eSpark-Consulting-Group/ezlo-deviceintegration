#include "zwave/resources/zwave_resources.h"

#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Zwave {
namespace Resources {

namespace DB {

const std::vector< std::string > BackupTableNames = { TableNames::Devices,
                                                      TableNames::Provisioning,
                                                      TableNames::ClassicInclusionSecurity };

namespace TableNames {
const char* const Devices = "db.addons.zwave.devices";
const char* const DeviceSettings = "db.addons.zwave.device.settings";
const char* const Provisioning = "db.addons.zwave.provisioning";
const char* const ClassicInclusionSecurity = "db.addons.zwave.classic_inclusion_security";

}  // namespace TableNames

namespace Fields {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const Base64Data = "base64Data";

}  // namespace Fields

}  // namespace DB

namespace Fields {

namespace ZwaveInfo {

const char* const Module = "module";
const char* const ModuleFirmwareVersion = "module_firmware_version";
const char* const Locale = "locale";
const char* const ZwaveApiVersion = "zwave_api_version";
const char* const ZwaveApiCapabilities = "zwave_api_capabilities";
const char* const ChipType = "chip_type";
const char* const ChipVersion = "chip_version";
const char* const HomeId = "home_id";
const char* const NodeId = "node_id";
const char* const Port = "port";
const char* const Poll = "poll";
const char* const PollPowered = "powered";
const char* const PollBattery = "battery";
const char* const Roles = "roles";
const char* const RoleSIS = "sis";
const char* const RolePrimary = "primary";
const char* const RoleSecondary = "secondary";
const char* const RoleStatic = "static";
const char* const RoleInclusion = "inclusion";

}  // namespace ZwaveInfo

namespace Node {

const char* const Id = "id";
const char* const ManufacturerId = "manufacturer_id";
const char* const ProductId = "product_id";
const char* const ProductType = "product_type";
const char* const Security = "security";
const char* const IsReachable = "is_reachable";
const char* const GenericType = "generic_type";
const char* const Channels = "channels";
const char* const SpecificType = "specific_type";
const char* const SecurityClassStarted = "security_class_started";
const char* const BasicType = "basic_type";
const char* const IsBatteryPowered = "is_battery_powered";
const char* const IconType = "icon_type";
const char* const IsSmartInclude = "is_smart_include";
const char* const IsRediscoverRequired = "is_rediscover_required";
const char* const WakeUpInterval = "wake_up_interval";
const char* const CommandSpan = "command_span";

}  // namespace Node

namespace VersionInfo {

const char* const LibraryType = "library_type";
const char* const ZwaveProtocolVersion = "zwave_protocol_version";
const char* const HardwareVersion = "hardware_version";
const char* const FirmwareVersions = "firmware_versions";
const char* const SdkVersion = "sdk_version";
const char* const ApplicationFrameworkApiVersion = "application_framework_api_version";
const char* const HostInterfaceVersion = "host_interface_version";
const char* const ProtocolVersion = "protocol_version";
const char* const ApplicationVersion = "application_version";
const char* const ApplicationFrameworkApiBuildNumber = "application_framework_api_build_number";
const char* const HostInterfaceBuildNumber = "host_interface_build_number";
const char* const ProtocolBuildNumber = "protocol_build_number";
const char* const ApplicationBuildNumber = "application_build_number";

}  // namespace VersionInfo

namespace Channel {

const char* const Id = "id";
const char* const Classes = "classes";

}  // namespace Channel

namespace Class {

const char* const Id = "id";
const char* const Version = "version";
const char* const Type = "type";
const char* const ResetSupported = "reset_supported";
const char* const Scales = "scales";
const char* const ComponentIds = "component_ids";
const char* const Opstates = "opstates";
const char* const Modes = "modes";
const char* const SensorTypes = "sensor_types";
const char* const Sensors = "sensors";
const char* const SensorType = "sensor_type";
const char* const Size = "size";
const char* const ReadOnly = "read_only";
const char* const Scale = "scale";
const char* const MinScale = "min_scale";
const char* const MinValue = "min_value";
const char* const MaxScale = "max_scale";
const char* const MaxValue = "max_value";
const char* const DefaultValue = "default_value";
const char* const Setpoints = "setpoints";
const char* const SupportedUsersCount = "supported_users_count";
const char* const Capabilities = "capabilities";
const char* const ChecksumSupported = "checksum_supported";
const char* const MasterCodeSupported = "master_code_supported";
const char* const MasterCodeDeactivationSupported = "master_code_deactivation_supported";
const char* const SupportedKeys = "supported_keys";
const char* const SupportedKeypadModes = "supported_keypad_modes";
const char* const SupportedUserIdStatuses = "supported_user_id_statuses";
const char* const Notifications = "notifications";
const char* const NotificationEventTypes = "event_types";
const char* const UnknownNotificationEventTypes = "unknown_event_types";
const char* const IsNotificationV1Supported = "v1_supported";
const char* const Signals = "signals";
const char* const Parameters = "parameters";
const char* const Format = "format";
const char* const Name = "name";
const char* const Info = "info";
const char* const CentralSceneButtons = "buttons";
const char* const CentralSceneButtonId = "button_id";
const char* const CentralSceneButtonAttributes = "button_attributes";
const char* const AssociationGroupsCount = "association_groups_count";
const char* const AssociationGroups = "association_groups";
const char* const GroupId = "group_id";
const char* const NodeId = "node_id";
const char* const EndpointId = "endpoint_id";
const char* const GroupNodes = "group_nodes";
const char* const GroupNodesMaxCount = "group_nodes_max_count";
const char* const GroupName = "group_name";
const char* const Profile = "profile";
const char* const ProfileMsb = "profile_msb";
const char* const ProfileLsb = "profile_lsb";
const char* const Commands = "commands";
const char* const CCId = "class_id";
const char* const CommandId = "command_id";
const char* const WeekDaySlots = "week_day_slots";
const char* const YearDaySlots = "year_day_slots";
const char* const DailyRepeatingSlots = "daily_repeating_slots";
const char* const InsideHandleIds = "inside_handle_ids";
const char* const OutsideHandleIds = "outside_handle_ids";
const char* const DoorLockModes = "door_lock_modes";
const char* const TimedOperationsSupported = "timed_operations_supported";
const char* const DoorSupported = "door_supported";
const char* const BoltSupported = "bolt_supported";
const char* const LatchSupported = "latch_supported";
const char* const AutoRelockSupported = "auto_relock_supported";
const char* const BlockToBlockSupported = "block_to_block_supported";
const char* const TwistAssistSupported = "twist_assist_supported";
const char* const HoldAndReleaseSupported = "hold_and_release_supported";
const char* const FirmwareIds = "firmware_ids";
const char* const IsSupported = "is_supported";
const char* const InitialLockInfo = "initial_lock_info";
const char* const IsRestricted = "restricted";
const char* const Hint = "hint";
const char* const ZWaveAllianceLockingId = "zwave_alliance_locking_id";
const char* const ManufacturerId = "manufacturer_id";
const char* const MaxWakeUpInterval = "max_wake_up_interval";
const char* const MinWakeUpInterval = "min_wake_up_interval";
const char* const StepWakeUpInterval = "step_wake_up_interval";

const char* const SupportedBlockIdsCount = "supported_block_ids_count";
const char* const SupportedScheduleIdsCount = "supported_schedule_ids_count";
const char* const ScheduleBlocks = "schedule_blocks";

const char* const BlockId = "block_id";
const char* const Classes = "classes";
const char* const SupportedCcCommands = "supported_cc_commands ";

const char* const SupportEnable = "support_enable";
const char* const FallbackSupport = "fallback_support";
const char* const StartTimeSupport = "start_time_support";
const char* const OverrideSupport = "override_support";
const char* const SupportedOverrideTypes = "supported_override_types";

}  // namespace Class

namespace Sound {

const char* const Tones = "tones";
const char* const ToneId = "tone_id";
const char* const ToneName = "tone_name";
const char* const Duration = "duration";

}  // namespace Sound

}  // namespace Fields

namespace ScriptEvent {

namespace Param {

const char* const NodeId = "node_id";
const char* const ManufacturerId = "manufacturer_id";
const char* const ProductId = "product_id";
const char* const ProductType = "product_type";
const char* const ClassId = "class_id";
const char* const ChannelId = "channel_id";
const char* const DeviceId = "device_id";
const char* const Value = "value";
const char* const TargetValue = "target_value";
const char* const TargetDuration = "target_duration";
const char* const SensorType = "sensor_type";
const char* const Type = "type";
const char* const Scale = "scale";
const char* const Precision = "precision";
const char* const Colors = "colors";
const char* const TargetColors = "target_colors";
const char* const TargetDurations = "target_durations";
const char* const Modes = "modes";
const char* const Mode = "mode";
const char* const Key = "key";
const char* const DoorLockMode = "door_lock_mode";
const char* const DoorCondition = "door_condition";
const char* const LatchState = "latch_state";
const char* const BoltState = "bolt_state";
const char* const DoorState = "door_state";
const char* const LockTimeout = "lock_timeout";
const char* const TargetDoorLockMode = "target_door_lock_mode";
const char* const TargetDoorLockModeTimeout = "target_door_lock_mode_timeout";
const char* const EnabledInsideHandleIds = "enabled_inside_handle_ids";
const char* const EnabledOutsideHandleIds = "enabled_outside_handle_ids";
const char* const AutoRelockTimeout = "auto_relock_timeout";
const char* const HoldAndReleaseTimeout = "hold_and_release_timeout";
const char* const TwistAssistEnabled = "twist_assist_enabled";
const char* const BlockToBlockEnabled = "block_to_block_enabled";
const char* const UserId = "user_id";
const char* const LastUserId = "last_user_id";
const char* const SupportedUsersCount = "supported_users_count";
const char* const UserCodeRanges = "user_code_ranges";
const char* const UserStatus = "user_status";
const char* const UserPollingStatus = "user_polling_status";
const char* const UserCode = "user_code";
const char* const NotificationType = "notification_type";
const char* const NotificationTypeRawValue = "notification_type_raw_value";
const char* const EventType = "event_type";
const char* const RawBase64Value = "raw_base64_value";
const char* const V1AlarmLevel = "v1_alarm_level";
const char* const V1AlarmType = "v1_alarm_type";
const char* const ResetStatus = "status";
const char* const ActionType = "action_type";
const char* const ParameterNum = "parameter_num";
const char* const ParameterSize = "parameter_size";
const char* const ButtonId = "button_id";
const char* const SceneId = "scene_id";
const char* const Delay = "delay";
const char* const GroupId = "group_id";
const char* const GroupNodes = "group_nodes";
const char* const EndpointId = "endpoint_id";
const char* const Hours = "hours";
const char* const Minutes = "minutes";
const char* const Seconds = "seconds";
const char* const SlotId = "slot_id";
const char* const WeekDay = "weekDay";
const char* const WeekDays = "weekDays";
const char* const StartYear = "startYear";
const char* const StartMonth = "startMonth";
const char* const StartDay = "startDay";
const char* const StartHour = "startHour";
const char* const StartMin = "startMinute";
const char* const StopYear = "stopYear";
const char* const StopMonth = "stopMonth";
const char* const StopDay = "stopDay";
const char* const StopHour = "stopHour";
const char* const StopMin = "stopMinute";
const char* const DurationHour = "durationHour";
const char* const DurationMin = "durationMinute";
const char* const Status = "status";
const char* const LearningDevices = "devices";
const char* const CurrentLearnedDevice = "current";
const char* const ReconstructDevices = "devices";
const char* const CurrentReconstructedDevice = "current";
const char* const Error = "error";
const char* const Progress = "progress";
const char* const Target = "target";
const char* const Image = "image";
const char* const Checksum = "checksum";
const char* const SignalName = "signal_name";
const char* const OverrideType = "override_type";
const char* const LockInfo = "lock_info";
const char* const IsRestricted = "restricted";
const char* const Hint = "hint";
const char* const ZWaveAllianceLockingId = "zwave_alliance_locking_id";
const char* const ManufacturerSpecificData = "manufacturer_specific_data";
const char* const IsRequested = "is_requested";
const char* const VeraDeviceModel = "vera_device_model";
const char* const Interval = "interval";
const char* const SequenceNumber = "sequence_number";
const char* const KeyAttribute = "key_attribute";
const char* const Commands = "commands";

namespace Notification {

const char* const UndefinedNotificationType = "undefined";

}  // namespace Notification

namespace Schedule {

const char* const ScheduleId = "schedule_id";
const char* const ScheduleIdBlock = "block_id";
const char* const Yearday = "yearday";
const char* const StartYear = "start_year";
const char* const StartMonth = "start_month";
const char* const StartDayOfMonth = "start_day_of_month";
const char* const Weekdays = "weekdays";
const char* const Hourandminute = "hourandminute";
const char* const StartHour = "start_hour";
const char* const StartMinute = "start_minute";
const char* const ActivityStatus = "activity_status";
const char* const RecurrenceOffset = "recurrence_offset";
const char* const RecurrenceMode = "recurrence_mode";
const char* const Relative = "relative";
const char* const DurationType = "duration_type";
const char* const Duration = "duration";
const char* const ReportToFollow = "report_to_follow";
const char* const Commands = "commands";
const char* const Options = "options";

const char* const NumberSupportedScheduleIds = "number_supported_schedule_ids";
const char* const ReportsToFollow = "reports_to_follow";
const char* const OverrideSupport = "override_support";
const char* const ScheduleStasuses = "schedule_stasuses";

}  // namespace Schedule

namespace Sound {
const char* const ToneId = "tone_id";
const char* const DefaultToneId = "default_tone_id";
const char* const Volume = "volume";
}  // namespace Sound

namespace ActionStatus {

const char* const Invoked = "invoked";
const char* const Started = "started";
const char* const Failed = "failed";
const char* const Timeout = "timeout";
const char* const Finished = "finished";
const char* const InProgress = "in_progress";
const char* const Busy = "busy";
const char* const Empty = "empty";
const char* const Undefined = "undefined";

}  // namespace ActionStatus

}  // namespace Param

namespace Name {

const char* const FloatValueUpdated = "float_value_updated";
const char* const RgbValueUpdated = "rgb_value_updated";
const char* const ThermostatSetpointValueUpdated = "thermostat_setpoint_value_updated";
const char* const ThermostatOperatingStateValueUpdated = "thermostat_operating_state_value_updated";
const char* const ThermostatSetbackValueUpdated = "thermostat_setback_value_updated";
const char* const ThermostatModeUpdated = "thermostat_mode_updated";
const char* const ModuleReset = "module_reset";
const char* const ActionBusy = "action_busy";
const char* const NodeAdded = "node_added";
const char* const NodeUpdated = "node_updated";
const char* const NodeRemoved = "node_removed";
const char* const AllNodesRemoved = "all_nodes_removed";
const char* const AssociationUpdated = "association_updated";
const char* const UnknownNodeRemoved = "unknown_node_removed";
const char* const ReachableStateUpdated = "reachable_state_updated";
const char* const ValueUpdated = "value_updated";
const char* const TokenValueUpdated = "token_value_updated";
const char* const BinarySensorValueUpdated = "binary_sensor_value_updated";
const char* const BarrierSignalValueUpdated = "barrier_signal_value_updated";
const char* const ConfigurationValueUpdated = "configuration_value_updated";
const char* const ConfigurationRequestStatusUpdated = "configuration_request_status_updated";
const char* const AlarmSensorValueUpdated = "alarm_sensor_value_updated";
const char* const DoorLockStateUpdated = "door_lock_state_updated";
const char* const DoorLockConfigurationUpdated = "door_lock_configuration_updated";
const char* const MasterCodeUpdated = "master_code_updated";
const char* const UserCodeUpdated = "user_code_updated";
const char* const UserCodesChecksumUpdated = "user_codes_checksum_updated";
const char* const UserCodesKeypadModeUpdated = "user_codes_keypad_mode_updated";
const char* const MultilevelSensorValueUpdated = "multilevel_sensor_value_updated";
const char* const ButtonStateUpdated = "button_state_updated";
const char* const SceneActivated = "scene_activated";
const char* const SimpleAvControlSetReceived = "simple_av_control_set_received";
const char* const NotificationRecieved = "notification_recieved";
const char* const UnknownNotificationRecieved = "unknown_notification_recieved";
const char* const S2SelectAuthenticationModes = "s2_select_authentication_modes";
const char* const S2RequestDeviceSpecificKey = "s2_request_device_specific_key";
const char* const S2ShowDeviceSideKey = "s2_show_device_side_key";
const char* const Include = "include";
const char* const Exclude = "exclude";
const char* const Learn = "learn";
const char* const Reconstruct = "reconstruct";
const char* const ReconstructProgress = "reconstruct_progress";
const char* const ReconstructProgressError = "reconstruct_progress_error";
const char* const ErrorSuffix = "error";
const char* const OnHail = "hail";
const char* const OnNif = "nif";
const char* const ScheduleEntryLockWeekDayUpdated = "week_day_entry_lock_schedule_updated";
const char* const ScheduleEntryLockYearDayUpdated = "year_day_entry_lock_schedule_updated";
const char* const ScheduleEntryLockDailyRepeatingUpdated = "daily_repeating_entry_lock_schedule_updated";
const char* const ScheduleUpdated = "schedule_updated";
const char* const ScheduleStateUpdated = "schedule_state_updated";

const char* const SoundSwitchConfigurationUpdated = "sound_switch_configuration_updated";
const char* const SoundSwitchTonePlayUpdated = "sound_switch_tone_play_updated";
const char* const IncludeDeviceProgress = "include_device_progress";
const char* const RediscoverDeviceProgress = "rediscover_device_progress";
const char* const LearnShowKey = "learn_show_key";
const char* const LearnProgress = "learn_progress";
const char* const NodeFirmwareUpdateStatus = "node_firmware_update_status";
const char* const NodeFirmwareDownloadStatus = "node_firmware_download_status";
const char* const NodeCheckStatus = "node_check_status";
const char* const SisNodeUpdated = "sis_node_updated";
const char* const AntiTheftStateUpdated = "anti_theft_state_updated";
const char* const VeraMigrationFinished = "vera_migration_finished";
const char* const WakeUpIntervalUpdated = "wake_up_interval_updated";
}  // namespace Name

namespace VeraDeviceModel {

const char* const Devices = "devices";
const char* const ParentId = "id_parent";
const char* const AltId = "altid";
const char* const Id = "id";
const char* const DeviceType = "device_type";
const char* const Invisible = "invisible";
const char* const Category = "category_num";

}  // namespace VeraDeviceModel

}  // namespace ScriptEvent

namespace RPC {

const char* const StartInclude = "startInclude";
const char* const StartExclude = "startExclude";
const char* const StopInclude = "stopInclude";
const char* const StopExclude = "stopExclude";

const char* const StartLearn = "startLearn";
const char* const StopLearn = "stopLearn";
const char* const SetAuthenticationMode = "setAuthenticationMode";
const char* const SetDeviceSpecificKey = "setDeviceSpecificKey";
const char* const SetValue = "setValue";
const char* const SetRgbValue = "setRgbValue";
const char* const SetSetpointValue = "setSetpointValue";
const char* const RequestValue = "requestValue";
const char* const RequestSetpointValue = "requestSetpointValue";
const char* const Reset = "reset";

const char* const ProvisioningList = "hub.addons.zwave.nodes.provisioning.list";
const char* const ProvisioningAdd = "hub.addons.zwave.nodes.provisioning.add";
const char* const ProvisioningSet = "hub.addons.zwave.nodes.provisioning.set";
const char* const ProvisioningRemove = "hub.addons.zwave.nodes.provisioning.remove";

}  // namespace RPC

namespace RPCParams {

const char* const NodeId = "nodeId";
const char* const NodeIds = "nodeIds";
const char* const ClassId = "classId";
const char* const ChannelId = "channelId";
const char* const UserId = "userId";
const char* const SlotId = "slotId";
const char* const Enable = "enable";
const char* const GroupId = "groupId";
const char* const ComponentId = "componentId";
const char* const UserIdStatus = "userIdStatus";
const char* const UserCode = "userCode";
const char* const MasterCode = "masterCode";
const char* const Value = "value";
const char* const Modes = "modes";
const char* const Mode = "mode";
const char* const Key = "key";
const char* const SetpointType = "setpointType";
const char* const Scale = "scale";
const char* const Command = "command";
const char* const ParamNumber = "param_number";
const char* const ParamSize = "param_size";
const char* const SoundDisablingMode = "disablingMode";
const char* const NotificationsMask = "notificationsMask";
const char* const SensorType = "sensorType";
const char* const Delay = "delay";
const char* const DestinationNode = "destinationNode";
const char* const DestinationEndpoint = "destinationEndpoint";
const char* const WeekDay = "weekDay";
const char* const WeekDays = "weekDays";
const char* const StartYear = "startYear";
const char* const StartMonth = "startMonth";
const char* const StartDay = "startDay";
const char* const StartHour = "startHour";
const char* const StartMin = "startMinute";
const char* const StopYear = "stopYear";
const char* const StopMonth = "stopMonth";
const char* const StopDay = "stopDay";
const char* const StopHour = "stopHour";
const char* const StopMin = "stopMinute";
const char* const Duration = "duration";
const char* const DurationHour = "durationHour";
const char* const DurationMin = "durationMinute";
const char* const RequestDelay = "requestDelay";
const char* const DeviceSettingFilePath = "deviceSettingFilePath";
const char* const EnabledInsideHandleIds = "enabledInsideHandleIds";
const char* const EnabledOutsideHandleIds = "enabledOutsideHandleIds";
const char* const LockTimeout = "lockTimeout";
const char* const AutoRelockTimeout = "autoRelockTimeout";
const char* const HoldAndReleaseTimeout = "holdAndReleaseTimeout";
const char* const TwistAssistEnabled = "twistAssistEnabled";
const char* const BlockToBlockEnabled = "blockToBlockEnabled";
const char* const Target = "target";
const char* const Checksum = "checksum";
const char* const Image = "image";
const char* const Firmwares = "firmwares";
const char* const Path = "path";
const char* const NotificationType = "notificationType";
const char* const NotificationEvent = "notificationEvent";
const char* const SignalName = "signalName";
const char* const OverrideType = "overrideType";
const char* const UseFixedWakeUpInterval = "useFixedWakeUpInterval";
const char* const WakeUpInterval = "wakeUpInterval";
const char* const AlwaysOnDevicesInterval = "alwaysOnDevicesInterval";
const char* const Interval = "interval";
const char* const AttemptsCount = "attemptsCount";
const char* const CcToCheck = "ccToCheck";
const char* const MagicCode = "magicCode";
const char* const Hint = "hint";
const char* const DeviceTypeId = "device_type_id";
const char* const ManufacturerSpecificData = "manufacturer_specific_data";
const char* const Configuration = "configuration";
const char* const Association = "association";
const char* const Force = "force";
const char* const UnsetPinCodesCountToStop = "unsetPinCodesCountToStop";

namespace Schedule {

const char* const ScheduleId = "schedule_id";
const char* const ScheduleIdBlock = "block_id";
const char* const Yearday = "yearday";
const char* const StartYear = "start_year";
const char* const StartMonth = "start_month";
const char* const StartDayOfMonth = "start_day_of_month";
const char* const Weekdays = "weekdays";
const char* const Hourandminute = "hourandminute";
const char* const StartHour = "start_hour";
const char* const StartMinute = "start_minute";
const char* const AidRoControl = "aid_ro_control";
const char* const RecurrenceOffset = "recurrence_offset";
const char* const RecurrenceMode = "recurrence_mode";
const char* const Relative = "relative";
const char* const DurationType = "duration_type";
const char* const Duration = "duration";
const char* const ReportToFollow = "report_to_follow";
const char* const Commands = "commands";
const char* const Options = "options";

const char* const ScheduleState = "schedule_state";

const char* const ScheduleBlocks = "schedule_blocks";
const char* const ScheduleTimeSupport = "start_time_support";
}  // namespace Schedule

}  // namespace RPCParams

namespace AuthModes {

const char* const AccessControl = "accessControl";
const char* const Authenticated = "authenticated";
const char* const Unauthenticated = "unauthenticated";
const char* const S0 = "s0";
const char* const ClientSide = "clientSide";

}  // namespace AuthModes

}  // namespace Resources
}  // namespace Zwave
}  // namespace HubPlatform
