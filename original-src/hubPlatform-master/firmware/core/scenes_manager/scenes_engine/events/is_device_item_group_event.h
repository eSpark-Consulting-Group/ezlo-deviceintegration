#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "scenes_manager/scenes_engine/events/item_event_base_factory.h"
#include <memory>

namespace SceneEngine {

class SceneException;

class IsDeviceItemGroupEvent : public ItemEventBase {
public:
    explicit IsDeviceItemGroupEvent();

    bool compare( const ItemValue& value ) const override;

    std::string getMethodName() const override;
};

using IsDeviceItemGroupEventPtr = std::shared_ptr< IsDeviceItemGroupEvent >;

}  // namespace SceneEngine