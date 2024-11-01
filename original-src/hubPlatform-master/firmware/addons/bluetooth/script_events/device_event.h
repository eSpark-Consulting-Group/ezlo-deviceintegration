#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {

class DeviceEvent : public Services::ScriptEvent {
public:
    DeviceEvent( std::string eventName, std::string deviceAddress );

    Json toJson() const override;

private:
    std::string m_deviceAddress;
};

}  // namespace HubPlatform
