#pragma once

#include <string>
#include <chrono>

namespace HubPlatform {
namespace Tests {
namespace Resources {

extern const std::string MQTTGatewayTopic;
extern const std::string MQTTGatewayAddonName;
extern const std::string BrokerPort;

extern const std::chrono::seconds MQTTConnectTimeout;
extern const std::chrono::seconds MQTTMessageTimeout;

namespace RPC {
extern const std::string Request;
extern const std::string MQTTRequest;
extern const std::string Response;
extern const std::string MQTTResponse;
extern const std::string Broadcast;
extern const std::string MQTTBroadcast;
}  // namespace RPC

}  // namespace Resources
}  // namespace Tests
}  // namespace HubPlatform
