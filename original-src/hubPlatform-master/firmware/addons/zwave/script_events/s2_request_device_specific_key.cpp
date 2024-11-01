#include "zwave/script_events/s2_request_device_specific_key.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

S2RequestDeviceSpecificKey::S2RequestDeviceSpecificKey( std::vector< std::uint16_t > pubKey )
: S2BaseDeviceKeyEvent( Resources::ScriptEvent::Name::S2RequestDeviceSpecificKey, std::move( pubKey ) ) {}

}  // namespace Zwave
}  // namespace HubPlatform
