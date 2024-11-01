#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class DeviceRemoved : public DeviceEvent {
public:
    explicit DeviceRemoved( std::string deviceAddress )
    : DeviceEvent( "device_removed", std::move( deviceAddress ) ) {}
};

}  // namespace HubPlatform
