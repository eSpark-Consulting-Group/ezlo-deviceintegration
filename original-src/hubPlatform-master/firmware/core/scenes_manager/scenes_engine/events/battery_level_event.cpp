#include "scenes_manager/scenes_engine/events/battery_level_event.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

BatteryLevelEvent::BatteryLevelEvent( Comparator comparator, const ItemValueWrapper& itemValue )
: m_comparator( std::move( comparator ) )
, m_batteryLevel( std::move( itemValue ) ) {}

std::string BatteryLevelEvent::getMethodName() const {
    return Model::Resources::MethodNames::isBatteryLevel;
}

bool BatteryLevelEvent::compare( int level ) const {
    ItemValueWrapper valueItem{ Json( level ) };
    return m_comparator( valueItem, m_batteryLevel );
}

}  // namespace SceneEngine
