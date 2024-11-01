#include "zwave/script_events/s2_show_device_side_key.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

S2ShowDeviceSideKey::S2ShowDeviceSideKey( std::vector< std::uint16_t > pubKey )
: S2BaseDeviceKeyEvent( Resources::ScriptEvent::Name::S2ShowDeviceSideKey, std::move( pubKey ) ) {}

}  // namespace Zwave
}  // namespace HubPlatform
