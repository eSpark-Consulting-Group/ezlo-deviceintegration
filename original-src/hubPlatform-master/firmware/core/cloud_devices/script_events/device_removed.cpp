#include "cloud_devices/script_events/device_removed.h"
#include "cloud_devices/resources/cloud_devices_resources.h"

namespace HubPlatform {
namespace CloudDevices {

DeviceRemoved::DeviceRemoved( std::string deviceId )
: Services::ScriptEvent( Resources::ScriptEvent::Name::DeviceRemoved )
, m_deviceId( std::move( deviceId ) ) {}

Json DeviceRemoved::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Fields::Id ] = m_deviceId;
    return result;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
