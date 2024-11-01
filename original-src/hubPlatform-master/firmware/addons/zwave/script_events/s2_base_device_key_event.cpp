#include "zwave/script_events/s2_base_device_key_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

S2BaseDeviceKeyEvent::S2BaseDeviceKeyEvent( std::string eventName, std::vector< std::uint16_t > pubKey )
: Services::ScriptEvent( std::move( eventName ) )
, m_pubKey( std::move( pubKey ) ) {}

Json S2BaseDeviceKeyEvent::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::Key ] = m_pubKey;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
