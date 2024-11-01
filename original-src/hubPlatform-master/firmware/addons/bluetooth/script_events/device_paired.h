#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class DevicePaired : public DeviceEvent {
public:
    explicit DevicePaired( std::string deviceAddress )
    : DeviceEvent( "device_paired", std::move( deviceAddress ) ) {}
};

}  // namespace HubPlatform
