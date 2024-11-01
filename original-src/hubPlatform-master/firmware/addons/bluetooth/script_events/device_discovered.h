#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class DeviceDiscovered : public DeviceEvent {
public:
    explicit DeviceDiscovered( std::string deviceAddress )
    : DeviceEvent( "device_discovered", std::move( deviceAddress ) ) {}
};

}  // namespace HubPlatform
