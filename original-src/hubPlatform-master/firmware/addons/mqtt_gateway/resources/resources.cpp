#include "mqtt_gateway/resources/resources.h"

#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace MQTTGateway {
namespace Resources {

namespace MessageFields {
const char* const MessageClass = "msg_class";
const char* const MessageSubclass = "msg_subclass";
const char* const MessageGroup = "msg_group";
const char* const UUID = "uuid";
const char* const Sender = "sender";
const char* const RequestBody = "requestBody";
const char* const ResponseBody = "responseBody";
const char* const Result = "result";
}  // namespace MessageFields

namespace MQTTMessageClasses {
const char* const Request = "request";
const char* const Response = "response";
const char* const Event = "event";
}  // namespace MQTTMessageClasses

namespace MQTT {
const char* const BrokerURI = "tcp://127.0.0.1:5012";
const char* const EnvBrokerPort = "MQTT_BROKER_PORT";
const char* const AddonTopic = "mqtt.gateway";
const size_t MaxTopicNameSize = 32;  // NOTE: This value matches spread group limit MAX_GROUP_NAME in "sp.h"
const size_t MaxMessageSize = 15 * 1024;
}  // namespace MQTT

namespace DB {

namespace TableNames {
const char* const RegisteredClientsDBName = "temp.db.addons.mqtt_gateway.registered_clients";
}  // namespace TableNames

}  // namespace DB

}  // namespace Resources
}  // namespace MQTTGateway
}  // namespace HubPlatform
