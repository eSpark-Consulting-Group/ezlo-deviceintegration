#pragma once

#include "zwave/script_events/s2_base_device_key_event.h"

namespace HubPlatform {
namespace Zwave {

class S2RequestDeviceSpecificKey final : public S2BaseDeviceKeyEvent {
public:
    S2RequestDeviceSpecificKey( std::vector< std::uint16_t > pubKey );
};

}  // namespace Zwave
}  // namespace HubPlatform
