#include "scenes_manager/scenes_engine/events/firmware_update_state_event.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

FirmwareUpdateStateEvent::FirmwareUpdateStateEvent( FirmwareUpdateStateEvent::State state )
: m_state( std::move( state ) ) {}

std::string FirmwareUpdateStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isFirmwareUpdateState;
}

bool FirmwareUpdateStateEvent::compare( const FirmwareUpdateStateEvent::State& state ) const {
    if( m_state == "started" && m_state == state ) {
        return true;
    }
    else if( m_state == "updating" && state == "process" ) {
        return true;
    }
    else if( m_state == "done" && state == "finished" ) {
        return true;
    }

    return false;
}

}  // namespace SceneEngine
