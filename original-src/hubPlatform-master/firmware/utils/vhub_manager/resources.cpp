#include "vhub_manager/resources.h"

namespace HubPlatform {
namespace VhubManager {
namespace Resources {

namespace MQTT {
const char* const brokerURI = "tcp://127.0.0.1:51883";
const char* const vhubManagerTopic = "vhub_manager";
const char* const provisionPageTopic = "fwm_2_provisionpage";
}  // namespace MQTT

namespace RPC {
const char* const provisionInfoGet = "provision.info.get";
const char* const provisionDataNew = "provision.data.new";
const char* const provisionDataDelete = "provision.data.delete";
const char* const vhubFirmwareStop = "vhub.firmware.stop";
const char* const vhubFirmwareStart = "vhub.firmware.start";
const char* const vhubStatusGet = "vhub.status.get";
const char* const vhubStatusChanged = "vhub.status.changed";
}  // namespace RPC

}  // namespace Resources
}  // namespace VhubManager
}  // namespace HubPlatform
