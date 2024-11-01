#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace CloudDevices {

class DeviceAdded : public Services::ScriptEvent {
public:
    explicit DeviceAdded( std::string deviceId );

    Json toJson() const override;

private:
    std::string m_deviceId;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
