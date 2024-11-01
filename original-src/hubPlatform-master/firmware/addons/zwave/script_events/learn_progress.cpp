#include "zwave/script_events/learn_progress.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

LearnProgress::LearnProgress( uint8_t devicesAmount, uint8_t currentDeviceIndex )
: Services::ScriptEvent( Resources::ScriptEvent::Name::LearnProgress )
, m_devicesAmount( devicesAmount )
, m_currentDeviceIndex( currentDeviceIndex ) {}

Json LearnProgress::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::LearningDevices ] = m_devicesAmount;
    result[ Param::CurrentLearnedDevice ] = m_currentDeviceIndex;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
