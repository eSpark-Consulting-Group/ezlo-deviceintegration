#include "zigbee/script_events/include_device_progress.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

IncludeDeviceProgress::IncludeDeviceProgress( uint8_t progress )
: Services::ScriptEvent( Resources::ScriptEvent::Name::IncludeDeviceProgress )
, m_progress( progress ) {}

Json IncludeDeviceProgress::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::Value ] = m_progress;
    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
