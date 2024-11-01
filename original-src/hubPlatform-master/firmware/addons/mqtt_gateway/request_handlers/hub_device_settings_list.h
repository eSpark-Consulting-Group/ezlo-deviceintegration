#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace MQTTGateway {
namespace RequestHandlers {

class HubDeviceSettingsList {
public:
    HubDeviceSettingsList( Services::Service& service );
    Json process( Json& params );

private:
    Services::Service& m_service;
};

}  // namespace RequestHandlers
}  // namespace MQTTGateway
}  // namespace HubPlatform
