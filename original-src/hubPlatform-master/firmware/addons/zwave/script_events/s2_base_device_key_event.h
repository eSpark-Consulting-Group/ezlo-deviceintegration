#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class S2BaseDeviceKeyEvent : public Services::ScriptEvent {
public:
    S2BaseDeviceKeyEvent( std::string eventName, std::vector< std::uint16_t > pubKey );

    Json toJson() const override;

private:
    std::vector< std::uint16_t > m_pubKey;
};

}  // namespace Zwave
}  // namespace HubPlatform
