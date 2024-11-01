#include "cloud_devices/resources/cloud_devices_resources.h"

#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace CloudDevices {
namespace Resources {
namespace DB {

const std::vector< std::string > BackupTableNames = { TableNames::Versions,
                                                      TableNames::Devices,
                                                      TableNames::Subscriptions };

namespace TableNames {

const char* const Versions = "db.addons.cloud_devices.versions";
const char* const Devices = "db.addons.cloud_devices.devices";
const char* const Subscriptions = HaCloudSubscriptionsDBName;

}  // namespace TableNames
namespace Fields {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const Version = "version";
const char* const Devices = "devices";

}  // namespace Fields

namespace SubscriptionFields {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const DeviceId = "deviceId";
const char* const ItemId = "itemId";
const char* const Mappings = "mappings";
const char* const Capability = "capability";
const char* const Variable = "variable";
const char* const GatewayName = "gatewayName";

}  // namespace SubscriptionFields

namespace Values {

const char* const DevicesTag = "devices";

}  // namespace Values
}  // namespace DB
namespace API {
namespace General {

const char* const Api = "api";
const char* const Method = "method";
const char* const Params = "params";

}  // namespace General
namespace Common {

const char* const Id = "id";

}  // namespace Common
namespace DevicesList {

const char* const Version = "version";
const char* const NotModified = "not_modified";
const char* const Devices = "devices";

}  // namespace DevicesList
namespace DeviceEvent {

const char* const Version = "version";
const char* const Devices = "devices";

}  // namespace DeviceEvent
namespace SendCommand {

const char* const Uuid = "uuid";
const char* const Capability = "capability";
const char* const Command = "command";
const char* const Parameters = "parameters";

}  // namespace SendCommand
}  // namespace API
namespace Model {
namespace Device {

const char* const Id = "id";
const char* const Capabilities = "capabilities";
const char* const Reachable = "reachable";

}  // namespace Device
}  // namespace Model
namespace ConfigKeys {

const char* const CloudRequestTimeout = "cloud_devices/cloud/requestTimeout";
const char* const ReaskOnErrorDelay = "cloud_devices/cloud/reaskOnErrorDelay";

}  // namespace ConfigKeys
namespace ScriptEvent {
namespace Name {

const char* const CommandResponse = "command_response";
const char* const DeviceAdded = "device_added";
const char* const DeviceRemoved = "device_removed";
const char* const DeviceUpdated = "device_updated";

}  // namespace Name
namespace Fields {

const char* const Id = "id";
const char* const CommandId = "command_id";
const char* const Result = "result";
const char* const Error = "error";

}  // namespace Fields
}  // namespace ScriptEvent

namespace SubscriptionGateway {

const char* const GatewayName = "AbstractSubscriptions";
const char* const GatewayPluginId = "AbstractSubscriptionsPluginId";

}  // namespace SubscriptionGateway

namespace AbstractDeviceProperty {

const char* const DeviceType = "abstract";
const char* const DeviceTypeId = "abstract";
const char* const DeviceCategory = "abstract";

}  // namespace AbstractDeviceProperty

namespace CloudRPCParams {

const char* const CommandId = "command_id";
const char* const DeviceId = "device_id";
const char* const ItemId = "item_id";
const char* const Capability = "capability";
const char* const Variable = "variable";
const char* const Command = "command";
const char* const Params = "params";
const char* const Abstracts = "abstracts";
const char* const Mappings = "mappings";
const char* const Value = "value";
const char* const AbstractUUID = "abstract_uuid";
const char* const GatewayName = "gateway_name";
const char* const ResponseType = "response_type";
const char* const ResponseData = "response_data";

const char* const GeofencingDispatch = "geofencing_dispatch";
const char* const GeofencingGateway = "ezlo_geofencing";

}  // namespace CloudRPCParams

namespace RPCParams {

const char* const DeviceId = "deviceId";
const char* const ItemId = "itemId";
const char* const ValueType = "valueType";
const char* const Value = "value";
const char* const AbstractUUID = "abstractUUID";
const char* const Mappings = "mappings";
const char* const Capability = "capability";
const char* const Variable = "variable";
const char* const DictionaryType = "dictionaryType";
const char* const Enum = "enum";
const char* const MaxValue = "maxValue";
const char* const MinValue = "minValue";
const char* const GatewayName = "gatewayName";

}  // namespace RPCParams
namespace CloudMethods {

const char* const CloudControllerSubscribeAbstracts = "cloud.controller_subscribe_abstracts";

}  // namespace CloudMethods

namespace GatewayBuiltInScripts {
const char* const SetItemValueScript = "cored:set_item_value";
}  // namespace GatewayBuiltInScripts

namespace GatewayScripts {
namespace ScriptCommand {
const char* const CreateAbstractSubscription = "createAbstractSubscriptionCommand";
const char* const UpdateAbstractSubscription = "updateAbstractSubscriptionCommand";
const char* const DeleteAbstractSubscription = "deleteAbstractSubscriptionCommand";
const char* const DispatchAbstractSubscription = "dispatchAbstractSubscriptionCommand";
}  // namespace ScriptCommand

namespace Commands {
const char* const CommandNameField = "command";
const char* const CommandParamsField = "params";
const char* const StoreSubscriptionRecord = "storeSubscriptionRecord";
const char* const RemoveSubscriptionRecord = "removeSubscriptionRecord";
}  // namespace Commands

namespace Fields {
const char* const Request = "request";
const char* const SubscriptionRecord = "subscriptionRecord";
const char* const MappingOperations = "mappingOperations";
const char* const MappingOperationAdd = "add";
const char* const MappingOperationUpdate = "update";
const char* const MappingOperationRemove = "remove";
const char* const MappingOperationKeep = "keep";
}  // namespace Fields
}  // namespace GatewayScripts

namespace Abstract {
namespace Geofencing {
const char* const CapabilityGeofencesAll = "geofences_all";
const char* const VariableGeofences = "geofences";
}  // namespace Geofencing
}  // namespace Abstract

}  // namespace Resources
}  // namespace CloudDevices
}  // namespace HubPlatform
