#pragma once

namespace HubPlatform {
namespace VhubManager {
namespace Resources {

namespace MQTT {
extern const char* const brokerURI;
extern const char* const vhubManagerTopic;
extern const char* const provisionPageTopic;
}  // namespace MQTT

namespace RPC {
extern const char* const provisionInfoGet;
extern const char* const provisionDataNew;
extern const char* const provisionDataDelete;
extern const char* const vhubFirmwareStop;
extern const char* const vhubFirmwareStart;
extern const char* const vhubStatusGet;
extern const char* const vhubStatusChanged;
}  // namespace RPC

}  // namespace Resources
}  // namespace VhubManager
}  // namespace HubPlatform
