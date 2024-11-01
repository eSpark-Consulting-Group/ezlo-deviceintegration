#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace CloudDevices {

class DeviceUpdated : public Services::ScriptEvent {
public:
    explicit DeviceUpdated( std::string deviceId, JsonObject deviceDiffJson );

    Json toJson() const override;

private:
    std::string m_deviceId;
    JsonObject m_deviceDiffJson;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
