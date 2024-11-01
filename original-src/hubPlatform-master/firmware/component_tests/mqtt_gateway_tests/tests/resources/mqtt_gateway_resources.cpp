#include "mqtt_gateway_tests/tests/resources/mqtt_gateway_resources.h"

namespace HubPlatform {
namespace Tests {
namespace Resources {

const std::string MQTTGatewayTopic = "mqtt.gateway";
const std::string MQTTGatewayAddonName = "addon.mqtt_gateway";
const std::string BrokerPort = "5012";

const std::chrono::seconds MQTTConnectTimeout{ 30 };
const std::chrono::seconds MQTTMessageTimeout{ 10 };

namespace RPC {

const std::string Request = R"(
{
    "msg_subclass": "hub.status.get",
    "result": {
        "requestBody": __REQUEST_BODY__,
        "sender": "__SENDER__",
        "uuid": "__UUID__"
    }
})";

const std::string MQTTRequest = R"(
{
    "msg_group": "__MSG_GROUP__",
    "msg_class": "request",
    "msg_subclass": "__MSG_SUBCLASS__",
    "result": {
        "requestBody": __REQUEST_BODY__,
        "sender": "__SENDER__",
        "uuid": "__UUID__"
    }
})";

const std::string Response = R"(
{
    "msg_subclass": "ResponseResult",
    "result": {
        "responseBody": __RESPONSE_BODY__,
        "uuid": "__UUID__"
    }
})";

const std::string MQTTResponse = R"(
{
    "msg_group": "__MSG_GROUP__",
    "msg_class": "response",
    "msg_subclass": "ResponseResult",
    "result": {
        "responseBody": __RESPONSE_BODY__,
        "uuid": "__UUID__"
    }
})";

const std::string Broadcast = R"(
{
    "msg_subclass": "__MSG_SUBCLASS__",
    "result": __RESULT__
})";

const std::string MQTTBroadcast = R"(
{
    "msg_group": "__MSG_GROUP__",
    "msg_class": "event",
    "msg_subclass": "__MSG_SUBCLASS__",
    "result": __RESULT__
})";

}  // namespace RPC

}  // namespace Resources
}  // namespace Tests
}  // namespace HubPlatform
