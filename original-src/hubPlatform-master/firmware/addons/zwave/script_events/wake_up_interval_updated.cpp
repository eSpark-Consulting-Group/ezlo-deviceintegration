#include "zwave/script_events/wake_up_interval_updated.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

WakeUpIntervalUpdated::WakeUpIntervalUpdated( uint8_t nodeId, const std::chrono::seconds& interval )
: NodeEvent( Resources::ScriptEvent::Name::WakeUpIntervalUpdated, nodeId )
, m_interval( interval ) {}

Json WakeUpIntervalUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = NodeEvent::toJson();
    result[ Param::Interval ] = m_interval.count();

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
