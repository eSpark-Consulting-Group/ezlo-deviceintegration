#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class DeviceUnpaired : public DeviceEvent {
public:
    explicit DeviceUnpaired( std::string deviceAddress )
    : DeviceEvent( "device_unpaired", std::move( deviceAddress ) ) {}
};

}  // namespace HubPlatform
