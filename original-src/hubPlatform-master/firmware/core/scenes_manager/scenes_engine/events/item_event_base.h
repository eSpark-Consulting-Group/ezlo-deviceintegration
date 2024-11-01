#pragma once
// Extracted from event_base.h/cpp
#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/value_source_key.h"
#include "scenes_manager/scenes_engine/events/item_value_wrapper.h"

namespace SceneEngine {

class ItemEventBase : public EventBase {
public:
    using DeviceId = std::string;
    using ItemValue = ItemValueWrapper;

    virtual bool compare( const ItemValue& value ) const = 0;
    virtual void checkValue( const ItemValue& value ) const {}
};

using ItemEventBasePtr = std::shared_ptr< ItemEventBase >;

}  // namespace SceneEngine
