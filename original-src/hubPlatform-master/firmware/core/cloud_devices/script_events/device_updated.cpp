#include "cloud_devices/script_events/device_updated.h"
#include "cloud_devices/common/device_converters.h"
#include "cloud_devices/resources/cloud_devices_resources.h"

namespace HubPlatform {
namespace CloudDevices {

DeviceUpdated::DeviceUpdated( std::string deviceId, JsonObject deviceDiffJson )
: Services::ScriptEvent( Resources::ScriptEvent::Name::DeviceUpdated )
, m_deviceId( std::move( deviceId ) )
, m_deviceDiffJson( std::move( deviceDiffJson ) ) {}

Json DeviceUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Fields::Id ] = m_deviceId;
    convertDeviceFromDbToModelFormat( m_deviceDiffJson, result );
    return result;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
