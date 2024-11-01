#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class LearnProgress final : public Services::ScriptEvent {
public:
    explicit LearnProgress( uint8_t devicesAmount, uint8_t currentDeviceIndex );

    Json toJson() const override;

private:
    uint8_t m_devicesAmount;
    uint8_t m_currentDeviceIndex;
};

}  // namespace Zwave
}  // namespace HubPlatform
