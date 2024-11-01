#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace CloudDevices {
namespace Resources {
namespace DB {

extern const std::vector< std::string > BackupTableNames;

namespace TableNames {

extern const char* const Versions;
extern const char* const Devices;
extern const char* const Subscriptions;

}  // namespace TableNames
namespace Fields {

extern const char* const Id;
extern const char* const Version;
extern const char* const Devices;

}  // namespace Fields

namespace SubscriptionFields {
extern const char* const Id;
extern const char* const DeviceId;
extern const char* const ItemId;
extern const char* const Mappings;
extern const char* const Capability;
extern const char* const Variable;
extern const char* const GatewayName;
}  // namespace SubscriptionFields

namespace Values {

extern const char* const DevicesTag;

}  // namespace Values
}  // namespace DB
namespace API {
namespace General {

extern const char* const Api;
extern const char* const Method;
extern const char* const Params;

}  // namespace General
namespace Common {

extern const char* const Id;

}  // namespace Common
namespace DevicesList {

extern const char* const Version;
extern const char* const NotModified;
extern const char* const Devices;

}  // namespace DevicesList
namespace DeviceEvent {

extern const char* const Version;
extern const char* const Devices;

}  // namespace DeviceEvent
namespace SendCommand {

extern const char* const Uuid;
extern const char* const Capability;
extern const char* const Command;
extern const char* const Parameters;

}  // namespace SendCommand
}  // namespace API
namespace Model {
namespace Device {

extern const char* const Id;
extern const char* const Capabilities;
extern const char* const Reachable;

}  // namespace Device
}  // namespace Model
namespace ConfigKeys {

extern const char* const CloudRequestTimeout;
extern const char* const ReaskOnErrorDelay;

}  // namespace ConfigKeys
namespace ScriptEvent {
namespace Name {

extern const char* const CommandResponse;
extern const char* const DeviceAdded;
extern const char* const DeviceRemoved;
extern const char* const DeviceUpdated;

}  // namespace Name
namespace Fields {

extern const char* const Id;
extern const char* const CommandId;
extern const char* const Result;
extern const char* const Error;

}  // namespace Fields
}  // namespace ScriptEvent

namespace SubscriptionGateway {
extern const char* const GatewayName;
extern const char* const GatewayPluginId;
}  // namespace SubscriptionGateway

namespace AbstractDeviceProperty {

extern const char* const DeviceType;
extern const char* const DeviceTypeId;
extern const char* const DeviceCategory;

}  // namespace AbstractDeviceProperty

namespace CloudRPCParams {

extern const char* const CommandId;
extern const char* const DeviceId;
extern const char* const ItemId;
extern const char* const Capability;
extern const char* const Variable;
extern const char* const Command;
extern const char* const Params;
extern const char* const Abstracts;
extern const char* const Mappings;
extern const char* const Value;
extern const char* const AbstractUUID;
extern const char* const GatewayName;
extern const char* const ResponseType;
extern const char* const ResponseData;

extern const char* const GeofencingDispatch;
extern const char* const GeofencingGateway;

}  // namespace CloudRPCParams

namespace RPCParams {

extern const char* const DeviceId;
extern const char* const ItemId;
extern const char* const ValueType;
extern const char* const Value;
extern const char* const AbstractUUID;
extern const char* const Mappings;
extern const char* const Capability;
extern const char* const Variable;
extern const char* const DictionaryType;
extern const char* const Enum;
extern const char* const MaxValue;
extern const char* const MinValue;
extern const char* const GatewayName;

}  // namespace RPCParams
namespace CloudMethods {

extern const char* const CloudControllerSubscribeAbstracts;

}  // namespace CloudMethods

namespace GatewayBuiltInScripts {
extern const char* const SetItemValueScript;
}  // namespace GatewayBuiltInScripts

namespace GatewayScripts {
namespace ScriptCommand {
extern const char* const CreateAbstractSubscription;
extern const char* const UpdateAbstractSubscription;
extern const char* const DeleteAbstractSubscription;
extern const char* const DispatchAbstractSubscription;
}  // namespace ScriptCommand

namespace Commands {
extern const char* const CommandNameField;
extern const char* const CommandParamsField;
extern const char* const StoreSubscriptionRecord;
extern const char* const RemoveSubscriptionRecord;
}  // namespace Commands

namespace Fields {
extern const char* const Request;
extern const char* const SubscriptionRecord;
extern const char* const MappingOperations;
extern const char* const MappingOperationAdd;
extern const char* const MappingOperationUpdate;
extern const char* const MappingOperationRemove;
extern const char* const MappingOperationKeep;
}  // namespace Fields

}  // namespace GatewayScripts

namespace Abstract {
namespace Geofencing {
extern const char* const CapabilityGeofencesAll;
extern const char* const VariableGeofences;
}  // namespace Geofencing
}  // namespace Abstract

}  // namespace Resources
}  // namespace CloudDevices
}  // namespace HubPlatform
