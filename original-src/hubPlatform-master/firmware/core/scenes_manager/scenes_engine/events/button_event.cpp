#include "scenes_manager/scenes_engine/events/button_event.h"
#include "cored/model/resources/cored_resources.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "json_utils/json_utils.h"

namespace SceneEngine {

ButtonStateEvent::ButtonStateEvent( const ButtonState& state )
: m_buttonState( state ) {}

ButtonStateEvent::ButtonStateEvent( ButtonStateEvent::ButtonState&& state )
: m_buttonState( std::move( state ) ) {}

std::string ButtonStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isButtonState;
}

bool ButtonStateEvent::compare( const ItemValue& value ) const {
    using namespace HubPlatform::CoreD::Model::Resources;
    const auto state = json_get_str( value.getValue(), ButtonStateValueFields::ButtonStateFieldName );
    return m_buttonState == state;
}

}  // namespace SceneEngine
