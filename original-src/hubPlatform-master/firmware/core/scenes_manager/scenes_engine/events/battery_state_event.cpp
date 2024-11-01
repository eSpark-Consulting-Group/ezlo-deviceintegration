#include "scenes_manager/scenes_engine/events/battery_state_event.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

BatteryStateEvent::BatteryStateEvent( BatteryState state )
: m_batteryState( std::move( state ) ) {}

std::string BatteryStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isBatteryState;
}

bool BatteryStateEvent::compare( const std::string& state ) const {
    return m_batteryState == state;
}

}  // namespace SceneEngine
