#pragma once

namespace HubPlatform {
namespace ScenesManager {

namespace EventBlockTypes {
extern const char* const When;
extern const char* const Exceptions;
}  // namespace EventBlockTypes

namespace ActionBlockTypes {
extern const char* const Then;
extern const char* const Else;
extern const char* const ThenGroups;
extern const char* const ElseGroups;
}  // namespace ActionBlockTypes

namespace ActionFields {

namespace Fields {
extern const char* const ResetType;
extern const char* const Block;
extern const char* const Group;
extern const char* const SceneId;
extern const char* const BlockId;
extern const char* const Type;
extern const char* const Name;
extern const char* const Control;
extern const char* const Exception;
}  // namespace Fields

namespace ExpressionFields {
extern const char* const Name;
extern const char* const Type;
extern const char* const Code;
extern const char* const ValueType;
extern const char* const Error;
extern const char* const Variable;
extern const char* const Metadata;
extern const char* const Params;
extern const char* const Item;
extern const char* const Expression;
}  // namespace ExpressionFields

namespace HubBackupFields {
extern const char* const Action;
extern const char* const BackupId;
extern const char* const Key;
extern const char* const Uuid;
extern const char* const Create;
extern const char* const Restore;

}  // namespace HubBackupFields

namespace ForwardNotificationFields {
extern const char* const NotificationTemplate;
extern const char* const NotificationBlock;
extern const char* const Device;
extern const char* const Item;
extern const char* const Targets;
}  // namespace ForwardNotificationFields

namespace Values {
extern const char* const ResetTypeSoft;
extern const char* const ResetTypeFactory;
extern const char* const BlockReference;
extern const char* const ExpressionReference;
}  // namespace Values

namespace ActionNames {
extern const char* const RebootHub;
extern const char* const ResetHub;
extern const char* const ResetLatch;
extern const char* const ResetSceneLatches;
extern const char* const RunScene;
extern const char* const StopScene;
extern const char* const RunCustomScript;
extern const char* const RunPluginScript;

extern const char* const SwitchHouseMode;
extern const char* const SetItemValue;
extern const char* const SetDeviceArmed;
extern const char* const SendCloudAbstractCommand;
extern const char* const SendHttpRequest;
extern const char* const SetSceneState;
extern const char* const CloudAPI;
extern const char* const SetExpression;
extern const char* const SetVariable;
extern const char* const ToggleValue;
extern const char* const GroupSetItemValue;
extern const char* const GroupSetDeviceArmed;
extern const char* const GroupToggleValue;
extern const char* const HubBackupAction;
extern const char* const ForwardNotifications;
}  // namespace ActionNames

}  // namespace ActionFields

}  // namespace ScenesManager
}  // namespace HubPlatform
