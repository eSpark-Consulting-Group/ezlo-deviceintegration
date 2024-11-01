#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

DeviceEvent::DeviceEvent( std::string eventName, std::string deviceAddress )
: Services::ScriptEvent( std::move( eventName ) )
, m_deviceAddress( std::move( deviceAddress ) ) {}

Json DeviceEvent::toJson() const {
    auto result = Services::ScriptEvent::toJson();
    result[ "deviceAddress" ] = m_deviceAddress;
    return result;
}
}  // namespace HubPlatform
