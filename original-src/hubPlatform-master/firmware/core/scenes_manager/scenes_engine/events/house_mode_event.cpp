#include "scenes_manager/scenes_engine/events/house_mode_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

HouseModeEvent::HouseModeEvent( Modes modes )
: m_modes( std::move( modes ) ) {}

std::string HouseModeEvent::getMethodName() const {
    return Model::Resources::MethodNames::isHouseModeChanged;
}

bool HouseModeEvent::compare( const std::string& mode ) const {
    return m_modes.find( mode ) != m_modes.end();
}

}  // namespace SceneEngine
