#include "scenes_manager/scenes_engine/events/cloud_state_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

CloudStateEvent::CloudStateEvent( State state )
: m_expectedState( state ) {}

std::string CloudStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isCloudState;
}

bool CloudStateEvent::compare( bool isCloudConnected ) const {
    switch( m_expectedState ) {
    case State::Connected:
        return isCloudConnected;
    case State::Disconnected:
        return !isCloudConnected;
    }

    return false;
}

}  // namespace SceneEngine
