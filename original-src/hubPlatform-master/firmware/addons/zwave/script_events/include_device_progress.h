#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class IncludeDeviceProgress : public Services::ScriptEvent {
public:
    explicit IncludeDeviceProgress( uint8_t progress );

    Json toJson() const override;

private:
    uint8_t m_progress;
};

}  // namespace Zwave
}  // namespace HubPlatform
