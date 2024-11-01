#pragma once

#include <stddef.h>

namespace HubPlatform {
namespace MQTTGateway {
namespace Resources {

namespace MessageFields {
extern const char* const MessageClass;
extern const char* const MessageSubclass;
extern const char* const MessageGroup;
extern const char* const UUID;
extern const char* const Sender;
extern const char* const RequestBody;
extern const char* const ResponseBody;
extern const char* const Result;
}  // namespace MessageFields

namespace MQTTMessageClasses {
extern const char* const Request;
extern const char* const Response;
extern const char* const Event;
}  // namespace MQTTMessageClasses

namespace MQTT {
extern const char* const BrokerURI;
extern const char* const EnvBrokerPort;
extern const char* const AddonTopic;
extern const size_t MaxTopicNameSize;
extern const size_t MaxMessageSize;
}  // namespace MQTT

namespace DB {

namespace TableNames {
extern const char* const RegisteredClientsDBName;
}  // namespace TableNames

}  // namespace DB

}  // namespace Resources
}  // namespace MQTTGateway
}  // namespace HubPlatform
