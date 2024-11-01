#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class ReconstructProgressEvent : public Services::ScriptEvent {
public:
    ReconstructProgressEvent( uint8_t devices, uint8_t current );

    Json toJson() const override;

private:
    uint8_t m_devices;
    uint8_t m_current;
};

}  // namespace Zwave
}  // namespace HubPlatform
