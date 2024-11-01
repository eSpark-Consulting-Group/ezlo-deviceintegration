#include "cored/model/resources/cored_resources.h"
#include "hub_settings/resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace Resources {

namespace Constants {
const uint8_t LowerLimitColorValue = 0;
const uint8_t UpperLimitColorValue = 255;
}  // namespace Constants

namespace DBNames {
const std::vector< std::string > BackupTableNames = { Model::Resources::DBNames::GatewaysDB,
                                                      Model::Resources::DBNames::DevicesDB,
                                                      Model::Resources::DBNames::ItemsDB,
                                                      Model::Resources::DBNames::DevicesSettingsDB,
                                                      Model::Resources::DBNames::RoomsDB,
                                                      Model::Resources::DBNames::FeaturesDB,
                                                      Model::Resources::DBNames::SettingsDB,
                                                      Model::Resources::DBNames::GatewaysSettingsDB,
                                                      Model::Resources::DBNames::DeviceGroupsDB,
                                                      Model::Resources::DBNames::ItemGroupsDB,
                                                      HaHouseModesDBName,
                                                      HaHouseModesCommandTimestampsDBName,
                                                      HaHouseModesSecurityThreatsDBName,
                                                      HaHouseModesProtectButtonsDBName,
                                                      HaHouseModesCamerasDBName,
                                                      HaHouseModesProtectDevicesDBName,
                                                      HaContactIdsDbName,
                                                      HaSecurityConfigsDbName,
                                                      HaPoliciesDbName,
                                                      HUBSettings,
                                                      FirmwareCoredSwingerShutdownDevices,
                                                      FirmwareCoredSwingerShutdownTimeSeries };

const char* const GatewaysDB = HaGatewaysDBName;
const char* const DevicesDB = HaDevicesDBName;
const char* const ItemsDB = HaItemsDBName;
const char* const GatewaysSettingsDB = HaGatewaySettingsDBName;
const char* const DevicesSettingsDB = HaDeviceSettingsDBName;
const char* const TempDevicesDB = HaTempDevicesDBName;
const char* const TempItemsDB = HaTempItemsDBName;
const char* const RoomsDB = HaRoomsDBName;
const char* const RoomsOrderDB = HaRoomsOrderDBName;
const char* const FeaturesDB = CoreFeaturesDBName;
const char* const SettingsDB = CoreSettingsDBName;
const char* const DeviceGroupsDB = HaDeviceGroupsDBName;
const char* const ItemGroupsDB = HaItemGroupsDBName;
const char* const ContactIdsDB = HaContactIdsDbName;
const char* const SecurityConfigsDB = HaSecurityConfigsDbName;
const char* const PoliciesDB = HaPoliciesDbName;

}  // namespace DBNames

namespace GatewayFields {
const char* const IdFieldName = db::ID_FIELD_NAME.c_str();
const char* const PluginIdFieldName = "pluginId";
const char* const NameFieldName = "name";
const char* const StatusFieldName = "status";
const char* const ReadyFieldName = "ready";
const char* const ManualDeviceAddingFieldName = "manualDeviceAdding";
const char* const UnreachableReasonsFieldName = "unreachableReasons";
const char* const UnreachableActionsFieldName = "unreachableActions";
const char* const LabelFieldName = "label";
const char* const ReasonFieldName = "reason";
const char* const SetItemValueCommandFieldName = "setItemValueCommand";
const char* const SetItemValueResponsePolicyFieldName = "setItemValueResponsePolicy";
const char* const AddItemDictionaryValueCommandFieldName = "addItemDictionaryValueCommand";
const char* const SetItemDictionaryValueCommandFieldName = "setItemDictionaryValueCommand";
const char* const RemoveItemDictionaryValueCommandFieldName = "removeItemDictionaryValueCommand";
const char* const ClearItemDictionaryCommandFieldName = "clearItemDictionaryCommand";
const char* const SetGatewaySettingValueCommandFieldName = "setGatewaySettingValueCommand";
const char* const SetSettingValueCommandFieldName = "setSettingValueCommand";
const char* const SetSettingDictionaryValueCommandFieldName = "setSettingDictionaryValueCommand";
const char* const ResetSettingCommandFieldName = "resetSettingCommand";
const char* const ForceRemoveDeviceCommandFieldName = "forceRemoveDeviceCommand";
const char* const UpdateDeviceFirmwareCommandFieldName = "updateDeviceFirmwareCommand";
const char* const CheckDeviceCommandFieldName = "checkDeviceCommand";
const char* const CreateAbstractSubscriptionCommandFieldName = "createAbstractSubscriptionCommand";
const char* const UpdateAbstractSubscriptionCommandFieldName = "updateAbstractSubscriptionCommand";
const char* const DeleteAbstractSubscriptionCommandFieldName = "deleteAbstractSubscriptionCommand";
const char* const DispatchAbstractSubscriptionCommandFieldName = "dispatchAbstractSubscriptionCommand";
const char* const InfoFieldName = "info";
const char* const SettingsFieldName = "settings";
}  // namespace GatewayFields

namespace GatewayBuiltInScripts {
const char* const SetItemValueScript = "cored:set_item_value";
}

namespace SetItemValueResponsePolicy {
const char* const Auto = "auto";
const char* const Custom = "custom";
}  // namespace SetItemValueResponsePolicy

namespace GatewayStatus {
const char* const NotStarted = "not_started";
const char* const Starting = "starting";
const char* const Ready = "ready";
}  // namespace GatewayStatus

namespace ManualDeviceAdding {
const char* const No = "no";
const char* const Wizard = "wizard";
}  // namespace ManualDeviceAdding

namespace DeviceFields {
const char* const IdFieldName = db::ID_FIELD_NAME.c_str();
const char* const GatewayIdFieldName = "gatewayId";
const char* const TypeFieldName = "type";
const char* const DeviceTypeIdFieldName = "deviceTypeId";
const char* const CategoryFieldName = "category";
const char* const SubcategoryFieldName = "subcategory";
const char* const ParentDeviceIdFieldName = "parentDeviceId";
const char* const InfoFieldName = "info";
const char* const FirmwareFieldName = "firmware";
const char* const FirmwareIdFieldName = "id";
const char* const FirmwareVersionFieldName = "version";
const char* const BatteryPoweredFieldName = "batteryPowered";
const char* const ReachableFieldName = "reachable";
const char* const NameFieldName = "name";
const char* const RoomIdFieldName = "roomId";
const char* const ArmedFieldName = "armed";
const char* const SecurityFieldName = "security";
const char* const StatusFieldName = "status";
const char* const ReadyFieldName = "ready";
const char* const ServiceNotificationFieldName = "serviceNotification";
const char* const PersistentFieldName = "persistent";
const char* const HouseModesOptionsFieldName = "houseModesOptions";
const char* const ParentRoomFieldName = "parentRoom";
const char* const SeparateChildDevices = "separateChildDevices";
const char* const ProtectConfig = "protectConfig";

namespace HouseModesOptions {
const char* const DisarmedFieldName = "disarmed";
const char* const AlarmsOffFieldName = "alarmsOff";
const char* const CamerasOffFieldName = "camerasOff";
}  // namespace HouseModesOptions

}  // namespace DeviceFields

namespace ItemFields {
const char* const IdFieldName = db::ID_FIELD_NAME.c_str();
const char* const DeviceIdFieldName = "deviceId";
const char* const NameFieldName = "name";
const char* const TypeFieldName = "type";
const char* const HasGetterFieldName = "hasGetter";
const char* const HasSetterFieldName = "hasSetter";
const char* const ShowFieldName = "show";
const char* const ElementsMaxNumberFieldName = "elementsMaxNumber";
const char* const StringRestrictionFieldName = "stringRestriction";
const char* const UserCodeRestrictionFieldName = "userCodeRestriction";
const char* const UserCodeModesFieldName = "userCodeModes";
const char* const ValueFieldName = "value";
const char* const OldValueFieldName = "oldValue";
const char* const ValueMinFieldName = "valueMin";
const char* const ValueMaxFieldName = "valueMax";
const char* const ValueFormattedFieldName = "valueFormatted";
const char* const EnumFieldName = "enum";
const char* const OneWeekDayCost = "oneWeekDayCost";
const char* const OneShiftedWeekDayCost = "oneShiftedWeekDayCost";
const char* const ValueIndentFieldName = "valueIndent";
const char* const ValueIndentItemFieldName = "item";
const char* const ValueIndentMinItemFieldName = "minIndent";
const char* const InfoFieldName = "info";

namespace Response {
const char* const MinValueFieldName = "minValue";
const char* const MaxValueFieldName = "maxValue";
}  // namespace Response

}  // namespace ItemFields

namespace DeviceSettingFields {
const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const DeviceId = "deviceId";
const char* const ValueType = "valueType";
const char* const Label = "label";
const char* const Description = "description";
const char* const Value = "value";
const char* const ValueDefault = "valueDefault";
const char* const ValueMin = "valueMin";
const char* const ValueMax = "valueMax";
const char* const ValueStep = "valueStep";
const char* const Enum = "enum";
const char* const HasSetter = "hasSetter";
const char* const ElementsMaxNumber = "elementsMaxNumber";
const char* const Info = "info";
const char* const ZwaveParameterNumber = "zwave.parameter_number";
const char* const ZwaveParameterSize = "zwave.parameter_size";
const char* const SynchronizationStatus = "status";
const char* const TargetChannelSupported = "targetChannelSupported";
}  // namespace DeviceSettingFields

namespace PolicyFields {
const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const Name = "name";
const char* const Persistent = "persistent";
const char* const ProtectConfig = "protectConfig";
const char* const DeviceGroup = "deviceGroup";
const char* const MeshBots = "meshbots";
const char* const Sync = "sync";
}  // namespace PolicyFields

namespace GatewaySettingFields {
const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const GatewayId = "gatewayId";
const char* const ValueType = "valueType";
const char* const Label = "label";
const char* const Description = "description";
const char* const Value = "value";
const char* const ValueDefault = "valueDefault";
const char* const ValueMin = "valueMin";
const char* const ValueMax = "valueMax";
const char* const Enum = "enum";
const char* const HasSetter = "hasSetter";
const char* const Info = "info";
const char* const SynchronizationStatus = "status";
}  // namespace GatewaySettingFields

namespace ScalableFields {
const char* const Value = "value";
const char* const Scale = "scale";
}  // namespace ScalableFields

namespace TextElementFields {
const char* const LangTag = "lang_tag";
const char* const Text = "text";
}  // namespace TextElementFields

namespace RoomFields {
const char* const IdFieldName = db::ID_FIELD_NAME.c_str();
const char* const NameFieldName = "name";
}  // namespace RoomFields

namespace RoomRecords {
extern const char* const OrderRecordId = "000";
}

namespace DictionaryEditRequestFields {
const char* const NameFieldName = "name";
const char* const DeviceIdFieldName = "deviceId";
const char* const ElementFieldName = "element";
const char* const ValueTypeFieldName = "type";
const char* const ValueFieldName = "value";
const char* const OldElementName = "oldElement";
const char* const KeyFieldName = "key";
const char* const OperationFieldName = "operation";
}  // namespace DictionaryEditRequestFields

namespace DictionaryOperations {
const char* const Added = "added";
const char* const Updated = "updated";
const char* const Removed = "removed";
}  // namespace DictionaryOperations

namespace UsercodeValueFields {
const char* const UserNameFieldName = "name";
const char* const UserCodeFieldName = "code";
const char* const UserModeFieldName = "mode";
}  // namespace UsercodeValueFields

namespace LedSegmentValueFields {
const char* const SegmentNameFieldName = "name";
const char* const SegmentStartFieldName = "start";
const char* const SegmentEndFieldName = "end";
}  // namespace LedSegmentValueFields

namespace LedEffectValueFields {
const char* const EffectNameFieldName = "name";
const char* const EffectDescriptionFieldName = "description";
const char* const EffectHeadpieceUrlFieldName = "headpieceUrl";
}  // namespace LedEffectValueFields

namespace DailyIntervalFields {
const char* const StartDateTime = "startDateTime";
const char* const StopDateTime = "stopDateTime";
}  // namespace DailyIntervalFields

namespace WeeklyIntervalFields {
const char* const WeekDays = "weekDays";
const char* const StartTime = "startTime";
const char* const StopTime = "stopTime";
}  // namespace WeeklyIntervalFields

namespace ButtonStateValueFields {
const char* const ButtonIdFieldName = "button_number";
const char* const ButtonStateFieldName = "button_state";
}  // namespace ButtonStateValueFields

namespace SoundInfoValueFields {
const char* const Name = "name";
const char* const Duration = "duration";
}  // namespace SoundInfoValueFields

namespace ZwaveConfigurationValueFields {
const char* const Name = "name";
const char* const Description = "description";
const char* const Number = "number";
const char* const Size = "size";
const char* const Value = "value";
const char* const HasSetter = "hasSetter";
const char* const Format = "format";
}  // namespace ZwaveConfigurationValueFields

namespace AssociationGroupValueFields {
const char* const Name = "name";
const char* const Profile = "profile";
const char* const Channel = "channel";
const char* const MaxAssociationsNumber = "maxAssociationsNumber";
const char* const Commands = "commands";
}  // namespace AssociationGroupValueFields

namespace AssociationValueFields {
const char* const TargetNode = "targetNode";
const char* const TargetChannel = "targetChannel";
}  // namespace AssociationValueFields

namespace DetectableObjectBaseFields {
const char* const Name = "name";
const char* const Value = "value";
const char* const LastHotzone = "lastHotzone";
const char* const Date = "date";
const char* const ItemId = "itemId";
const char* const Id = "id";
}  // namespace DetectableObjectBaseFields

namespace CameraStreamValueFields {
const char* const localRtspUrl = "localRtspUrl";
const char* const cloudLiveStreamUrl = "cloudLiveStreamUrl";
}  // namespace CameraStreamValueFields

namespace CameraHotzoneValueFields {
const char* const Name = "name";
const char* const Detected = "detected";
const char* const Objects = "objects";
const char* const Persons = "persons";
const char* const QRCodes = "qrcodes";
}  // namespace CameraHotzoneValueFields

namespace HotzoneMatchValueFields {
const char* const Match = "match";
const char* const List = "list";
}  // namespace HotzoneMatchValueFields

namespace GeofenceValueFields {
const char* const Modified = "modified";
}  // namespace GeofenceValueFields

namespace UserLockOperation {
const int InvalidUserCodeId = -1;

namespace Fields {
const char* const UserId = "userId";
const char* const Action = "action";
}  // namespace Fields

namespace Actions {
const char* const Unknown = "unknown";
const char* const Lock = "lock";
const char* const Unlock = "unlock";
const char* const ValidUserOutsideOfSchedule = "valid_user_but_outside_of_schedule";
}  // namespace Actions
}  // namespace UserLockOperation

namespace ShutterSecondPart {
const char* const InvalidDeviceId = "";

namespace Fields {
const char* const DeviceId = "deviceId";
const char* const PartFunction = "partFunction";
}  // namespace Fields

namespace PartFunctions {
const char* const Unknown = "unknown";
const char* const Front = "front";
const char* const Back = "back";
}  // namespace PartFunctions
}  // namespace ShutterSecondPart

namespace UserCodeAction {
const int InvalidUserCodeId = -1;

namespace Fields {
const char* const UserId = "userId";
const char* const Action = "action";
}  // namespace Fields

namespace Actions {
const char* const Unknown = "unknown";
const char* const UserCodeEntered = "user_code_entered";
const char* const InvalidUserCodeEntered = "invalid_user_code_entered";
}  // namespace Actions
}  // namespace UserCodeAction

namespace RPC {
const char* const RemovePluginGatewaysRPC = "removePluginGateways";
const char* const RemoveDeviceRPC = "removeDevice";
const char* const RenameDeviceRPC = "renameDevice";

const char* const RemoveGatewayDevicesRPC = "removeGatewayDevices";
const char* const UpdateItemValueRPC = "updateItemValue";
const char* const UpdateItemFieldRPC = "updateItemField";
const char* const UpdateItemDictionaryValueRPC = "updateItemDictionaryValue";
const char* const UpdateSettingValueRPC = "updateSettingValue";
const char* const UpdateSettingDictionaryValueRPC = "updateSettingDictionaryValue";
const char* const UpdateSettingStatusRPC = "updateSettingStatus";
const char* const UpdateReachableStateRPC = "updateReachableStateValue";
const char* const UpdateDeviceStatusRPC = "updateDeviceStatusValue";
const char* const UpdateReadyStateRPC = "updateReadyStateValue";
const char* const UpdateDeviceNameRPC = "updateDeviceName";
const char* const UpdateDeviceRoomIdRPC = "updateDeviceRoomId";
const char* const ForceRemoveDeviceRPC = "removeDeviceForce";
}  // namespace RPC

namespace RPCParams {
const char* const GatewayId = "gatewayId";
const char* const Gateway = "gateway";
const char* const GatewayStatus = "status";
const char* const GatewayManualDeviceAdding = "manualDeviceAdding";
const char* const PluginId = "pluginId";
const char* const DeviceId = "deviceId";
const char* const Device = "device";
const char* const DeviceName = "name";
const char* const DeviceRoomId = "roomId";
const char* const DeviceStatus = "deviceStatus";
const char* const ItemId = "itemId";
const char* const Item = "item";
const char* const ItemType = "itemType";
const char* const ItemValue = "itemValue";
const char* const ItemFieldValue = "itemFieldValue";
const char* const ItemField = "itemField";
const char* const ItemValueMin = "itemValueMin";
const char* const ItemValueMax = "itemValueMax";
const char* const ItemValueFormatted = "itemValueFormatted";
const char* const ReachableState = "reachableState";
const char* const ReadyState = "readyState";
const char* const ItemShow = "itemShow";
const char* const ItemDictionaryValue = "itemDictionaryValue";
const char* const ItemDictionaryId = "itemDictionaryId";
const char* const SettingId = "settingId";
const char* const Setting = "setting";
const char* const DictionaryKey = "dictionaryKey";
const char* const DictionaryValue = "dictionaryValue";
const char* const ValueType = "valueType";
const char* const Value = "value";
const char* const SynchronizationStatus = "status";
const char* const Process = "process";
const char* const ProcessResult = "result";
const char* const VeraDeviceId = "veraDeviceId";
const char* const DeviceData = "deviceData";
const char* const SourceField = "source";
const char* const SeparateChildDevices = "separateChildDevices";
const char* const ParentRoom = "parentRoom";
}  // namespace RPCParams

namespace HouseModeAlarmStatus {
const char* const Begin = "begin";
const char* const Done = "done";
const char* const Main = "main";
const char* const Idle = "idle";
const char* const Canceled = "canceled";
const char* const EntryDelay = "entryDelay";
const char* const Bypass = "bypass";
}  // namespace HouseModeAlarmStatus

namespace HouseModeAlarmPhase {
const char* const Idle = "idle";
const char* const Main = "alarmed.main";
const char* const EntryDelay = "alarmed.entryDelay";
const char* const SwitchTo = "switchTo";
}  // namespace HouseModeAlarmPhase

namespace ItemSource {
const char* const HouseMode = "house_mode";
const char* const Scene = "scene";
const char* const TypeField = "type";
}  // namespace ItemSource

const char* const ServiceName = "cored";
const char* const FileNamePartialSuffix = ".partial";
const char* const BackupFileExtension = ".json";
const char* const CORED_VERSION = "1.1";

namespace System {
const char* const SyncNotification = "syncNotification";
const char* const FirmwareTimestampMs = "fwTimestampMs";
}  // namespace System

namespace Rooms {
bool ParentRoomDefault = true;
}  // namespace Rooms

namespace FeaturesFields {
const char* const Id = "_id";
const char* const Name = "name";
const char* const Status = "status";
const char* const Editable = "editable";
const char* const IsCustom = "is_custom";
const char* const Version = "version";
}  // namespace FeaturesFields

namespace FeatureStatus {
const char* const On = "on";
const char* const Off = "off";
}  // namespace FeatureStatus

namespace ProtectConfigName {
const char* const Default = "default";
}  // namespace ProtectConfigName

}  // namespace Resources
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
