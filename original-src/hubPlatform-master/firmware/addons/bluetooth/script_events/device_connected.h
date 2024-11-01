#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class DeviceConnected : public DeviceEvent {
public:
    explicit DeviceConnected( std::string deviceAddress )
    : DeviceEvent( "device_connected", std::move( deviceAddress ) ) {}
};

}  // namespace HubPlatform
