#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class DeviceDisconnected : public DeviceEvent {
public:
    explicit DeviceDisconnected( std::string deviceAddress )
    : DeviceEvent( "device_disconnected", std::move( deviceAddress ) ) {}
};

}  // namespace HubPlatform
