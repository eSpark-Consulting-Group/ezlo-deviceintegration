#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace CloudDevices {

class DeviceRemoved : public Services::ScriptEvent {
public:
    explicit DeviceRemoved( std::string deviceId );

    Json toJson() const override;

private:
    std::string m_deviceId;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
