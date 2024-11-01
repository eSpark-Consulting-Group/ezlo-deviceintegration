#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"

namespace SceneEngine {

class ButtonStateEvent : public ItemEventBase {
public:
    using ItemID = std::string;
    using ItemEventBase::ItemValue;
    using ButtonState = std::string;

    ButtonStateEvent( const ButtonState& state );
    ButtonStateEvent( ButtonState&& state );

    std::string getMethodName() const override;

    bool compare( const ItemValue& value ) const override;

private:
    ButtonState m_buttonState;
};

using ButtonStateEventPtr = std::shared_ptr< ButtonStateEvent >;

}  // namespace SceneEngine
