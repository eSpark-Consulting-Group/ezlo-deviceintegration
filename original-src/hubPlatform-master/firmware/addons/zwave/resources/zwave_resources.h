#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace Zwave {
namespace Resources {

namespace DB {

extern const std::vector< std::string > BackupTableNames;

namespace TableNames {

extern const char* const Devices;
extern const char* const DeviceSettings;
extern const char* const Provisioning;
extern const char* const ClassicInclusionSecurity;

}  // namespace TableNames

namespace Fields {

extern const char* const Id;
extern const char* const Base64Data;

}  // namespace Fields

}  // namespace DB

namespace Fields {

namespace ZwaveInfo {

extern const char* const Module;
extern const char* const ModuleFirmwareVersion;
extern const char* const Locale;
extern const char* const ZwaveApiVersion;
extern const char* const ZwaveApiCapabilities;
extern const char* const ChipType;
extern const char* const ChipVersion;
extern const char* const HomeId;
extern const char* const NodeId;
extern const char* const Port;
extern const char* const Poll;
extern const char* const PollPowered;
extern const char* const PollBattery;
extern const char* const Roles;
extern const char* const RoleSIS;
extern const char* const RolePrimary;
extern const char* const RoleSecondary;
extern const char* const RoleStatic;
extern const char* const RoleInclusion;

}  // namespace ZwaveInfo

namespace Node {

extern const char* const Id;
extern const char* const ManufacturerId;
extern const char* const ProductId;
extern const char* const ProductType;
extern const char* const Security;
extern const char* const IsReachable;
extern const char* const GenericType;
extern const char* const Channels;
extern const char* const SpecificType;
extern const char* const SecurityClassStarted;
extern const char* const BasicType;
extern const char* const IsBatteryPowered;
extern const char* const IconType;
extern const char* const IsSmartInclude;
extern const char* const IsRediscoverRequired;
extern const char* const WakeUpInterval;
extern const char* const CommandSpan;

}  // namespace Node

namespace VersionInfo {

extern const char* const LibraryType;
extern const char* const ZwaveProtocolVersion;
extern const char* const HardwareVersion;
extern const char* const FirmwareVersions;
extern const char* const SdkVersion;
extern const char* const ApplicationFrameworkApiVersion;
extern const char* const HostInterfaceVersion;
extern const char* const ProtocolVersion;
extern const char* const ApplicationVersion;
extern const char* const ApplicationFrameworkApiBuildNumber;
extern const char* const HostInterfaceBuildNumber;
extern const char* const ProtocolBuildNumber;
extern const char* const ApplicationBuildNumber;

}  // namespace VersionInfo

namespace Channel {

extern const char* const Id;
extern const char* const Classes;

}  // namespace Channel

namespace Class {

extern const char* const Id;
extern const char* const Version;
extern const char* const Type;
extern const char* const ResetSupported;
extern const char* const Scales;
extern const char* const ComponentIds;
extern const char* const Opstates;
extern const char* const Modes;
extern const char* const SensorTypes;
extern const char* const Sensors;
extern const char* const SensorType;
extern const char* const Size;
extern const char* const ReadOnly;
extern const char* const Scale;
extern const char* const MinScale;
extern const char* const MinValue;
extern const char* const MaxScale;
extern const char* const MaxValue;
extern const char* const DefaultValue;
extern const char* const Setpoints;
extern const char* const SupportedUsersCount;
extern const char* const ChecksumSupported;
extern const char* const MasterCodeSupported;
extern const char* const MasterCodeDeactivationSupported;
extern const char* const SupportedKeys;
extern const char* const SupportedKeypadModes;
extern const char* const SupportedUserIdStatuses;
extern const char* const Capabilities;
extern const char* const Notifications;
extern const char* const NotificationEventTypes;
extern const char* const UnknownDeviceEvent;
extern const char* const UnknownNotificationEventTypes;
extern const char* const IsNotificationV1Supported;
extern const char* const Signals;
extern const char* const Parameters;
extern const char* const Format;
extern const char* const Name;
extern const char* const Info;
extern const char* const CentralSceneButtons;
extern const char* const CentralSceneButtonId;
extern const char* const CentralSceneButtonAttributes;
extern const char* const AssociationGroupsCount;
extern const char* const AssociationGroups;
extern const char* const GroupId;
extern const char* const NodeId;
extern const char* const EndpointId;
extern const char* const GroupNodes;
extern const char* const GroupNodesMaxCount;
extern const char* const GroupName;
extern const char* const Profile;
extern const char* const ProfileMsb;
extern const char* const ProfileLsb;
extern const char* const Commands;
extern const char* const CCId;
extern const char* const CommandId;
extern const char* const WeekDaySlots;
extern const char* const YearDaySlots;
extern const char* const DailyRepeatingSlots;
extern const char* const InsideHandleIds;
extern const char* const OutsideHandleIds;
extern const char* const DoorLockModes;
extern const char* const TimedOperationsSupported;
extern const char* const DoorSupported;
extern const char* const BoltSupported;
extern const char* const LatchSupported;
extern const char* const AutoRelockSupported;
extern const char* const BlockToBlockSupported;
extern const char* const TwistAssistSupported;
extern const char* const HoldAndReleaseSupported;
extern const char* const FirmwareIds;
extern const char* const IsSupported;
extern const char* const InitialLockInfo;
extern const char* const IsRestricted;
extern const char* const Hint;
extern const char* const ZWaveAllianceLockingId;
extern const char* const ManufacturerId;
extern const char* const MaxWakeUpInterval;
extern const char* const MinWakeUpInterval;
extern const char* const StepWakeUpInterval;

extern const char* const SupportedScheduleIdsCount;
extern const char* const SupportedBlockIdsCount;
extern const char* const ScheduleBlocks;

extern const char* const BlockId;
extern const char* const Classes;
extern const char* const SupportedCcCommands;

extern const char* const SupportEnable;
extern const char* const FallbackSupport;
extern const char* const StartTimeSupport;
extern const char* const OverrideSupport;
extern const char* const SupportedOverrideTypes;

}  // namespace Class

namespace Sound {

extern const char* const Tones;
extern const char* const ToneId;
extern const char* const ToneName;
extern const char* const Duration;

}  // namespace Sound

}  // namespace Fields

namespace ScriptEvent {

namespace Param {

extern const char* const NodeId;
extern const char* const ManufacturerId;
extern const char* const ProductType;
extern const char* const ProductId;
extern const char* const ClassId;
extern const char* const ChannelId;
extern const char* const DeviceId;
extern const char* const Value;
extern const char* const TargetValue;
extern const char* const TargetDuration;
extern const char* const SensorType;
extern const char* const Type;
extern const char* const Scale;
extern const char* const Precision;
extern const char* const Colors;
extern const char* const TargetColors;
extern const char* const TargetDurations;
extern const char* const Mode;
extern const char* const Modes;
extern const char* const Key;
extern const char* const DoorLockMode;
extern const char* const DoorCondition;
extern const char* const LatchState;
extern const char* const BoltState;
extern const char* const DoorState;
extern const char* const LockTimeout;
extern const char* const TargetDoorLockMode;
extern const char* const TargetDoorLockModeTimeout;
extern const char* const EnabledInsideHandleIds;
extern const char* const EnabledOutsideHandleIds;
extern const char* const AutoRelockTimeout;
extern const char* const HoldAndReleaseTimeout;
extern const char* const TwistAssistEnabled;
extern const char* const BlockToBlockEnabled;
extern const char* const UserId;
extern const char* const LastUserId;
extern const char* const SupportedUsersCount;
extern const char* const UserCodeRanges;
extern const char* const UserStatus;
extern const char* const UserPollingStatus;
extern const char* const UserCode;
extern const char* const NotificationType;
extern const char* const NotificationTypeRawValue;
extern const char* const EventType;
extern const char* const RawBase64Value;
extern const char* const V1AlarmLevel;
extern const char* const V1AlarmType;
extern const char* const ResetStatus;
extern const char* const ActionType;
extern const char* const ParameterNum;
extern const char* const ParameterSize;
extern const char* const ButtonId;
extern const char* const SceneId;
extern const char* const Delay;
extern const char* const GroupId;
extern const char* const GroupNodes;
extern const char* const EndpointId;
extern const char* const Hours;
extern const char* const Minutes;
extern const char* const Seconds;
extern const char* const SlotId;
extern const char* const WeekDay;
extern const char* const WeekDays;
extern const char* const StartYear;
extern const char* const StartMonth;
extern const char* const StartDay;
extern const char* const StartHour;
extern const char* const StartMin;
extern const char* const StopYear;
extern const char* const StopMonth;
extern const char* const StopDay;
extern const char* const StopHour;
extern const char* const StopMin;
extern const char* const DurationHour;
extern const char* const DurationMin;
extern const char* const Status;
extern const char* const LearningDevices;
extern const char* const CurrentLearnedDevice;
extern const char* const ReconstructDevices;
extern const char* const CurrentReconstructedDevice;
extern const char* const Error;
extern const char* const Progress;
extern const char* const Target;
extern const char* const Image;
extern const char* const Checksum;
extern const char* const SignalName;
extern const char* const OverrideType;
extern const char* const LockInfo;
extern const char* const IsRestricted;
extern const char* const Hint;
extern const char* const ZWaveAllianceLockingId;
extern const char* const ManufacturerSpecificData;
extern const char* const IsRequested;
extern const char* const VeraDeviceModel;
extern const char* const Interval;
extern const char* const SequenceNumber;
extern const char* const KeyAttribute;
extern const char* const Commands;

namespace Notification {

extern const char* const UndefinedNotificationType;

}  // namespace Notification

namespace Schedule {

extern const char* const ScheduleId;
extern const char* const ScheduleIdBlock;
extern const char* const Yearday;
extern const char* const StartYear;
extern const char* const StartMonth;
extern const char* const StartDayOfMonth;
extern const char* const Weekdays;
extern const char* const Hourandminute;
extern const char* const StartHour;
extern const char* const StartMinute;
extern const char* const ActivityStatus;
extern const char* const RecurrenceOffset;
extern const char* const RecurrenceMode;
extern const char* const Relative;
extern const char* const DurationType;
extern const char* const Duration;
extern const char* const ReportToFollow;
extern const char* const Commands;
extern const char* const Options;

extern const char* const NumberSupportedScheduleIds;
extern const char* const ReportsToFollow;
extern const char* const OverrideSupport;
extern const char* const ScheduleStasuses;

}  // namespace Schedule

namespace Sound {

extern const char* const ToneId;
extern const char* const DefaultToneId;
extern const char* const Volume;

}  // namespace Sound

namespace ActionStatus {

extern const char* const Invoked;
extern const char* const Started;
extern const char* const Failed;
extern const char* const Timeout;
extern const char* const Finished;
extern const char* const InProgress;
extern const char* const Busy;
extern const char* const Empty;
extern const char* const Undefined;

}  // namespace ActionStatus

}  // namespace Param

namespace Name {

extern const char* const FloatValueUpdated;
extern const char* const RgbValueUpdated;
extern const char* const ThermostatSetpointValueUpdated;
extern const char* const ThermostatModeUpdated;
extern const char* const ThermostatOperatingStateValueUpdated;
extern const char* const ThermostatSetbackValueUpdated;
extern const char* const ModuleReset;
extern const char* const ActionBusy;
extern const char* const NodeAdded;
extern const char* const NodeUpdated;
extern const char* const NodeRemoved;
extern const char* const AllNodesRemoved;
extern const char* const AssociationUpdated;
extern const char* const UnknownNodeRemoved;
extern const char* const ReachableStateUpdated;
extern const char* const ValueUpdated;
extern const char* const TokenValueUpdated;
extern const char* const BinarySensorValueUpdated;
extern const char* const BarrierSignalValueUpdated;
extern const char* const ConfigurationValueUpdated;
extern const char* const ConfigurationRequestStatusUpdated;
extern const char* const AlarmSensorValueUpdated;
extern const char* const DoorLockStateUpdated;
extern const char* const DoorLockConfigurationUpdated;
extern const char* const MasterCodeUpdated;
extern const char* const UserCodeUpdated;
extern const char* const UserCodesChecksumUpdated;
extern const char* const UserCodesKeypadModeUpdated;
extern const char* const MultilevelSensorValueUpdated;
extern const char* const ButtonStateUpdated;
extern const char* const SceneActivated;
extern const char* const SimpleAvControlSetReceived;
extern const char* const NotificationRecieved;
extern const char* const UnknownNotificationRecieved;
extern const char* const S2SelectAuthenticationModes;
extern const char* const S2RequestDeviceSpecificKey;
extern const char* const S2ShowDeviceSideKey;
extern const char* const Include;
extern const char* const Exclude;
extern const char* const Learn;
extern const char* const Reconstruct;
extern const char* const ReconstructProgress;
extern const char* const ReconstructProgressError;
extern const char* const ErrorSuffix;
extern const char* const OnHail;
extern const char* const OnNif;
extern const char* const ScheduleEntryLockWeekDayUpdated;
extern const char* const ScheduleEntryLockYearDayUpdated;
extern const char* const ScheduleEntryLockDailyRepeatingUpdated;
extern const char* const ScheduleUpdated;
extern const char* const ScheduleStateUpdated;
extern const char* const SoundSwitchConfigurationUpdated;
extern const char* const SoundSwitchTonePlayUpdated;
extern const char* const IncludeDeviceProgress;
extern const char* const RediscoverDeviceProgress;
extern const char* const LearnShowKey;
extern const char* const LearnProgress;
extern const char* const NodeFirmwareUpdateStatus;
extern const char* const NodeFirmwareDownloadStatus;
extern const char* const NodeCheckStatus;
extern const char* const SisNodeUpdated;
extern const char* const AntiTheftStateUpdated;
extern const char* const VeraMigrationFinished;
extern const char* const WakeUpIntervalUpdated;
}  // namespace Name

namespace VeraDeviceModel {

extern const char* const Devices;
extern const char* const ParentId;
extern const char* const AltId;
extern const char* const Id;
extern const char* const DeviceType;
extern const char* const Invisible;
extern const char* const Category;

}  // namespace VeraDeviceModel

}  // namespace ScriptEvent

namespace RPC {

extern const char* const StartInclude;
extern const char* const StartExclude;
extern const char* const StopInclude;
extern const char* const StopExclude;

extern const char* const StartLearn;

extern const char* const StopLearn;
extern const char* const SetAuthenticationMode;
extern const char* const SetDeviceSpecificKey;
extern const char* const SetValue;
extern const char* const SetRgbValue;
extern const char* const SetSetpointValue;
extern const char* const RequestValue;
extern const char* const RequestSetpointValue;
extern const char* const Reset;

extern const char* const ProvisioningList;
extern const char* const ProvisioningAdd;
extern const char* const ProvisioningSet;
extern const char* const ProvisioningRemove;

}  // namespace RPC

namespace RPCParams {

extern const char* const NodeId;
extern const char* const NodeIds;
extern const char* const ClassId;
extern const char* const ChannelId;
extern const char* const ComponentId;
extern const char* const UserId;
extern const char* const SlotId;
extern const char* const Enable;
extern const char* const GroupId;
extern const char* const UserIdStatus;
extern const char* const UserCode;
extern const char* const MasterCode;
extern const char* const Value;
extern const char* const Modes;
extern const char* const Mode;
extern const char* const Key;
extern const char* const SetpointType;
extern const char* const Scale;
extern const char* const Command;
extern const char* const ParamNumber;
extern const char* const ParamSize;
extern const char* const SoundDisablingMode;
extern const char* const NotificationsMask;
extern const char* const SensorType;
extern const char* const Delay;
extern const char* const DestinationNode;
extern const char* const DestinationEndpoint;
extern const char* const WeekDay;
extern const char* const WeekDays;
extern const char* const StartYear;
extern const char* const StartMonth;
extern const char* const StartDay;
extern const char* const StartHour;
extern const char* const StartMin;
extern const char* const StopYear;
extern const char* const StopMonth;
extern const char* const StopDay;
extern const char* const StopHour;
extern const char* const StopMin;
extern const char* const Duration;
extern const char* const DurationHour;
extern const char* const DurationMin;
extern const char* const RequestDelay;
extern const char* const DeviceSettingFilePath;
extern const char* const EnabledInsideHandleIds;
extern const char* const EnabledOutsideHandleIds;
extern const char* const LockTimeout;
extern const char* const AutoRelockTimeout;
extern const char* const HoldAndReleaseTimeout;
extern const char* const TwistAssistEnabled;
extern const char* const BlockToBlockEnabled;
extern const char* const Firmwares;
extern const char* const Target;
extern const char* const Checksum;
extern const char* const Image;
extern const char* const Path;
extern const char* const NotificationType;
extern const char* const NotificationEvent;
extern const char* const SignalName;
extern const char* const OverrideType;
extern const char* const UseFixedWakeUpInterval;
extern const char* const WakeUpInterval;
extern const char* const AlwaysOnDevicesInterval;
extern const char* const Interval;
extern const char* const AttemptsCount;
extern const char* const CcToCheck;
extern const char* const MagicCode;
extern const char* const Hint;
extern const char* const ManufacturerSpecificData;
extern const char* const DeviceTypeId;
extern const char* const Configuration;
extern const char* const Association;
extern const char* const Force;
extern const char* const UnsetPinCodesCountToStop;

namespace Schedule {

extern const char* const ScheduleId;
extern const char* const ScheduleIdBlock;
extern const char* const Yearday;
extern const char* const StartYear;
extern const char* const StartMonth;
extern const char* const StartDayOfMonth;
extern const char* const Weekdays;
extern const char* const Hourandminute;
extern const char* const StartHour;
extern const char* const StartMinute;
extern const char* const AidRoControl;
extern const char* const RecurrenceOffset;
extern const char* const RecurrenceMode;
extern const char* const Relative;
extern const char* const DurationType;
extern const char* const Duration;
extern const char* const ReportToFollow;
extern const char* const Commands;
extern const char* const Options;

extern const char* const ScheduleState;
extern const char* const ScheduleBlocks;
extern const char* const ScheduleTimeSupport;
}  // namespace Schedule

}  // namespace RPCParams

namespace AuthModes {

extern const char* const AccessControl;
extern const char* const Authenticated;
extern const char* const Unauthenticated;
extern const char* const S0;
extern const char* const ClientSide;

}  // namespace AuthModes

}  // namespace Resources
}  // namespace Zwave
}  // namespace HubPlatform
