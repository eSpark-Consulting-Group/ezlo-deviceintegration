#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace Resources {

namespace Constants {
extern const uint8_t LowerLimitColorValue;
extern const uint8_t UpperLimitColorValue;
}  // namespace Constants

namespace DBNames {
extern const std::vector< std::string > BackupTableNames;

extern const char* const GatewaysDB;
extern const char* const DevicesDB;
extern const char* const TempDevicesDB;
extern const char* const ItemsDB;
extern const char* const GatewaysSettingsDB;
extern const char* const DevicesSettingsDB;
extern const char* const TempItemsDB;
extern const char* const RoomsDB;
extern const char* const RoomsOrderDB;
extern const char* const FeaturesDB;
extern const char* const SettingsDB;
extern const char* const DeviceGroupsDB;
extern const char* const ItemGroupsDB;
extern const char* const ContactIdsDB;
extern const char* const SecurityConfigsDB;
extern const char* const PoliciesDB;

}  // namespace DBNames

namespace GatewayFields {
extern const char* const IdFieldName;
extern const char* const PluginIdFieldName;
extern const char* const NameFieldName;
extern const char* const StatusFieldName;
extern const char* const ReadyFieldName;
extern const char* const ManualDeviceAddingFieldName;
extern const char* const UnreachableReasonsFieldName;
extern const char* const UnreachableActionsFieldName;
extern const char* const LabelFieldName;
extern const char* const ReasonFieldName;
extern const char* const SetItemValueCommandFieldName;
extern const char* const SetItemValueResponsePolicyFieldName;
extern const char* const AddItemDictionaryValueCommandFieldName;
extern const char* const SetItemDictionaryValueCommandFieldName;
extern const char* const RemoveItemDictionaryValueCommandFieldName;
extern const char* const ClearItemDictionaryCommandFieldName;
extern const char* const SetGatewaySettingValueCommandFieldName;
extern const char* const SetSettingValueCommandFieldName;
extern const char* const SetSettingDictionaryValueCommandFieldName;
extern const char* const ResetSettingCommandFieldName;
extern const char* const ForceRemoveDeviceCommandFieldName;
extern const char* const UpdateDeviceFirmwareCommandFieldName;
extern const char* const CheckDeviceCommandFieldName;
extern const char* const CreateAbstractSubscriptionCommandFieldName;
extern const char* const UpdateAbstractSubscriptionCommandFieldName;
extern const char* const DeleteAbstractSubscriptionCommandFieldName;
extern const char* const DispatchAbstractSubscriptionCommandFieldName;
extern const char* const InfoFieldName;
extern const char* const SettingsFieldName;
}  // namespace GatewayFields

namespace GatewayBuiltInScripts {
extern const char* const SetItemValueScript;
}

namespace SetItemValueResponsePolicy {
extern const char* const Auto;
extern const char* const Custom;
}  // namespace SetItemValueResponsePolicy

namespace GatewayStatus {
extern const char* const NotStarted;
extern const char* const Starting;
extern const char* const Ready;
}  // namespace GatewayStatus

namespace ManualDeviceAdding {
extern const char* const No;
extern const char* const Wizard;
}  // namespace ManualDeviceAdding

namespace DeviceFields {
extern const char* const IdFieldName;
extern const char* const GatewayIdFieldName;
extern const char* const TypeFieldName;
extern const char* const CategoryFieldName;
extern const char* const SubcategoryFieldName;
extern const char* const ParentDeviceIdFieldName;
extern const char* const DeviceTypeIdFieldName;
extern const char* const InfoFieldName;
extern const char* const FirmwareFieldName;
extern const char* const FirmwareIdFieldName;
extern const char* const FirmwareVersionFieldName;
extern const char* const BatteryPoweredFieldName;
extern const char* const ReachableFieldName;
extern const char* const NameFieldName;
extern const char* const RoomIdFieldName;
extern const char* const ArmedFieldName;
extern const char* const SecurityFieldName;
extern const char* const StatusFieldName;
extern const char* const ReadyFieldName;
extern const char* const ServiceNotificationFieldName;
extern const char* const PersistentFieldName;
extern const char* const HouseModesOptionsFieldName;
extern const char* const ParentRoomFieldName;
extern const char* const SeparateChildDevices;
extern const char* const ProtectConfig;

namespace HouseModesOptions {
extern const char* const DisarmedFieldName;
extern const char* const AlarmsOffFieldName;
extern const char* const CamerasOffFieldName;
}  // namespace HouseModesOptions

}  // namespace DeviceFields

namespace ItemFields {
extern const char* const IdFieldName;
extern const char* const DeviceIdFieldName;
extern const char* const NameFieldName;
extern const char* const TypeFieldName;
extern const char* const HasGetterFieldName;
extern const char* const HasSetterFieldName;
extern const char* const ShowFieldName;
extern const char* const ElementsMaxNumberFieldName;
extern const char* const StringRestrictionFieldName;
extern const char* const UserCodeRestrictionFieldName;
extern const char* const UserCodeModesFieldName;
extern const char* const ValueFieldName;
extern const char* const OldValueFieldName;
extern const char* const ValueMinFieldName;
extern const char* const ValueMaxFieldName;
extern const char* const ValueFormattedFieldName;
extern const char* const EnumFieldName;
extern const char* const OneWeekDayCost;
extern const char* const InfoFieldName;
extern const char* const OneShiftedWeekDayCost;
extern const char* const ValueIndentFieldName;
extern const char* const ValueIndentItemFieldName;
extern const char* const ValueIndentMinItemFieldName;

namespace Response {
// Replacements for ValueMinFieldName/ValueMaxFieldName in broadcasts (hub.item.added/hub.item.updated) and hub.*.list
// commands:
extern const char* const MinValueFieldName;
extern const char* const MaxValueFieldName;
}  // namespace Response
}  // namespace ItemFields

namespace DeviceSettingFields {
extern const char* const Id;
extern const char* const DeviceId;
extern const char* const ValueType;
extern const char* const Label;
extern const char* const Description;
extern const char* const Value;
extern const char* const ValueDefault;
extern const char* const ValueMin;
extern const char* const ValueMax;
extern const char* const ValueStep;
extern const char* const Enum;
extern const char* const HasSetter;
extern const char* const ElementsMaxNumber;
extern const char* const Info;
extern const char* const ZwaveParameterNumber;
extern const char* const ZwaveParameterSize;
extern const char* const SynchronizationStatus;
extern const char* const TargetChannelSupported;
}  // namespace DeviceSettingFields

namespace PolicyFields {
extern const char* const Id;
extern const char* const Name;
extern const char* const Persistent;
extern const char* const ProtectConfig;
extern const char* const DeviceGroup;
extern const char* const MeshBots;
extern const char* const Sync;
}  // namespace PolicyFields

namespace GatewaySettingFields {
extern const char* const Id;
extern const char* const GatewayId;
extern const char* const ValueType;
extern const char* const Label;
extern const char* const Description;
extern const char* const Value;
extern const char* const ValueDefault;
extern const char* const ValueMin;
extern const char* const ValueMax;
extern const char* const Enum;
extern const char* const HasSetter;
extern const char* const TargetChannelSupported;
extern const char* const Info;
extern const char* const SynchronizationStatus;
}  // namespace GatewaySettingFields

namespace ScalableFields {
extern const char* const Value;
extern const char* const Scale;
}  // namespace ScalableFields

namespace TextElementFields {
extern const char* const LangTag;
extern const char* const Text;
}  // namespace TextElementFields

namespace RoomFields {
extern const char* const IdFieldName;
extern const char* const NameFieldName;
}  // namespace RoomFields

namespace RoomRecords {
extern const char* const OrderRecordId;
}

namespace DictionaryEditRequestFields {
extern const char* const NameFieldName;
extern const char* const DeviceIdFieldName;
extern const char* const ElementFieldName;
extern const char* const ValueTypeFieldName;
extern const char* const ValueFieldName;
extern const char* const OldElementName;
extern const char* const KeyFieldName;
extern const char* const OperationFieldName;
extern const char* const DetectedIdsFieldName;
extern const char* const HotzoneFieldName;
}  // namespace DictionaryEditRequestFields

namespace DictionaryOperations {
extern const char* const Added;
extern const char* const Updated;
extern const char* const Removed;
}  // namespace DictionaryOperations

namespace UsercodeValueFields {
extern const char* const UserNameFieldName;
extern const char* const UserCodeFieldName;
extern const char* const UserModeFieldName;
}  // namespace UsercodeValueFields

namespace LedSegmentValueFields {
extern const char* const SegmentNameFieldName;
extern const char* const SegmentStartFieldName;
extern const char* const SegmentEndFieldName;
}  // namespace LedSegmentValueFields

namespace LedEffectValueFields {
extern const char* const EffectNameFieldName;
extern const char* const EffectDescriptionFieldName;
extern const char* const EffectHeadpieceUrlFieldName;
}  // namespace LedEffectValueFields

namespace DailyIntervalFields {
extern const char* const StartDateTime;
extern const char* const StopDateTime;
}  // namespace DailyIntervalFields

namespace WeeklyIntervalFields {
extern const char* const WeekDays;
extern const char* const StartTime;
extern const char* const StopTime;
}  // namespace WeeklyIntervalFields

namespace ButtonStateValueFields {
extern const char* const ButtonIdFieldName;
extern const char* const ButtonStateFieldName;
}  // namespace ButtonStateValueFields

namespace SoundInfoValueFields {
extern const char* const Name;
extern const char* const Duration;
}  // namespace SoundInfoValueFields

namespace ZwaveConfigurationValueFields {
extern const char* const Name;
extern const char* const Description;
extern const char* const Number;
extern const char* const Size;
extern const char* const Value;
extern const char* const HasSetter;
extern const char* const Format;
}  // namespace ZwaveConfigurationValueFields

namespace AssociationGroupValueFields {
extern const char* const Name;
extern const char* const Profile;
extern const char* const Channel;
extern const char* const MaxAssociationsNumber;
extern const char* const Commands;
}  // namespace AssociationGroupValueFields

namespace AssociationValueFields {
extern const char* const TargetNode;
extern const char* const TargetChannel;
}  // namespace AssociationValueFields

namespace DetectableObjectBaseFields {
extern const char* const Name;
extern const char* const Value;
extern const char* const LastHotzone;
extern const char* const Date;
extern const char* const ItemId;
extern const char* const Id;
}  // namespace DetectableObjectBaseFields

namespace CameraStreamValueFields {
extern const char* const localRtspUrl;
extern const char* const cloudLiveStreamUrl;
}  // namespace CameraStreamValueFields

namespace CameraHotzoneValueFields {
extern const char* const Name;
extern const char* const Detected;
extern const char* const Objects;
extern const char* const Persons;
extern const char* const QRCodes;
}  // namespace CameraHotzoneValueFields

namespace HotzoneMatchValueFields {
extern const char* const Match;
extern const char* const List;
}  // namespace HotzoneMatchValueFields

namespace GeofenceValueFields {
extern const char* const Modified;
}  // namespace GeofenceValueFields

namespace UserLockOperation {
extern const int InvalidUserCodeId;

namespace Fields {
extern const char* const UserId;
extern const char* const Action;
}  // namespace Fields

namespace Actions {
extern const char* const Unknown;
extern const char* const Lock;
extern const char* const Unlock;
extern const char* const ValidUserOutsideOfSchedule;
}  // namespace Actions
}  // namespace UserLockOperation

namespace ShutterSecondPart {
extern const char* const InvalidDeviceId;

namespace Fields {
extern const char* const DeviceId;
extern const char* const PartFunction;
}  // namespace Fields

namespace PartFunctions {
extern const char* const Unknown;
extern const char* const Front;
extern const char* const Back;
}  // namespace PartFunctions
}  // namespace ShutterSecondPart

namespace UserCodeAction {
extern const int InvalidUserCodeId;

namespace Fields {
extern const char* const UserId;
extern const char* const Action;
}  // namespace Fields

namespace Actions {
extern const char* const Unknown;
extern const char* const UserCodeEntered;
extern const char* const InvalidUserCodeEntered;
}  // namespace Actions
}  // namespace UserCodeAction

namespace RPC {
extern const char* const RemovePluginGatewaysRPC;
extern const char* const RemoveDeviceRPC;
extern const char* const RenameDeviceRPC;
extern const char* const RemoveGatewayDevicesRPC;
extern const char* const UpdateItemValueRPC;
extern const char* const UpdateItemFieldRPC;
extern const char* const UpdateItemDictionaryValueRPC;
extern const char* const UpdateSettingValueRPC;
extern const char* const UpdateSettingDictionaryValueRPC;
extern const char* const UpdateSettingStatusRPC;
extern const char* const UpdateReachableStateRPC;
extern const char* const UpdateDeviceStatusRPC;
extern const char* const UpdateReadyStateRPC;
extern const char* const UpdateDeviceNameRPC;
extern const char* const UpdateDeviceRoomIdRPC;
extern const char* const ForceRemoveDeviceRPC;
}  // namespace RPC

namespace RPCParams {
extern const char* const GatewayId;
extern const char* const Gateway;
extern const char* const GatewayStatus;
extern const char* const GatewayManualDeviceAdding;
extern const char* const PluginId;
extern const char* const DeviceId;
extern const char* const Device;
extern const char* const DeviceName;
extern const char* const DeviceRoomId;
extern const char* const DeviceStatus;
extern const char* const ItemId;
extern const char* const Item;
extern const char* const ItemType;
extern const char* const ItemValue;
extern const char* const ItemFieldValue;
extern const char* const ItemField;
extern const char* const ItemValueMin;
extern const char* const ItemValueMax;
extern const char* const ItemValueFormatted;
extern const char* const ReachableState;
extern const char* const ReadyState;
extern const char* const ItemShow;
extern const char* const ItemDictionaryValue;
extern const char* const ItemDictionaryId;
extern const char* const SettingId;
extern const char* const Setting;
extern const char* const DictionaryKey;
extern const char* const DictionaryValue;
extern const char* const ValueType;
extern const char* const Value;
extern const char* const SynchronizationStatus;
extern const char* const Process;
extern const char* const ProcessResult;
extern const char* const VeraDeviceId;
extern const char* const DeviceData;
extern const char* const SourceField;
extern const char* const SeparateChildDevices;
extern const char* const ParentRoom;

}  // namespace RPCParams

namespace ItemSource {
extern const char* const HouseMode;
extern const char* const Scene;
extern const char* const TypeField;
}  // namespace ItemSource

extern const char* const ServiceName;
extern const char* const FileNamePartialSuffix;
extern const char* const BackupFileExtension;
extern const char* const CORED_VERSION;

namespace System {
extern const char* const TimeZonesFolderPath;
extern const char* const SyncNotification;
extern const char* const FirmwareTimestampMs;
}  // namespace System

namespace Rooms {
extern bool ParentRoomDefault;
}  // namespace Rooms

namespace HouseModeAlarmStatus {
extern const char* const Begin;
extern const char* const Done;
extern const char* const Main;
extern const char* const Idle;
extern const char* const Canceled;
extern const char* const EntryDelay;
extern const char* const Bypass;
}  // namespace HouseModeAlarmStatus

namespace HouseModeAlarmPhase {
extern const char* const Main;
extern const char* const Idle;
extern const char* const EntryDelay;
extern const char* const SwitchTo;
}  // namespace HouseModeAlarmPhase

namespace FeaturesFields {
extern const char* const Id;
extern const char* const Name;
extern const char* const Status;
extern const char* const Editable;
extern const char* const IsCustom;
extern const char* const Version;
}  // namespace FeaturesFields

namespace FeatureStatus {
extern const char* const On;
extern const char* const Off;
}  // namespace FeatureStatus

namespace ProtectConfigName {
extern const char* const Default;
}  // namespace ProtectConfigName

}  // namespace Resources
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
