#pragma once

#include "service_interfaces/service.h"
#include "networkd/network_manager.h"
#include "networkd/remote_interface/network_statistics_settings_manager.h"
#include "networkd/remote_interface/network_remote_interface.h"

#include "cored/remote_interface/cored_interface.h"
#include "http/http.h"
#include "cloud_storage/response_dispatcher.h"
#include <cloud_storage/request_builder.h>

namespace HubPlatform {
namespace Network {
namespace CommunicationTest {
namespace ProgressIndicator {
const int TestStarted = 0;
const int TryToEnabledInterface = 10;
const int InterfaceIsEnabled = 30;
const int SendHTTPRequest = 50;
const int TestCompleted = 100;
}  // namespace ProgressIndicator

namespace InternalStatus {
const std::string Started = "started";
}
}  // namespace CommunicationTest
}  // namespace Network
}  // namespace HubPlatform
