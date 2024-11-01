#include "is_device_item_group_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

IsDeviceItemGroupEvent::IsDeviceItemGroupEvent() = default;

std::string IsDeviceItemGroupEvent::getMethodName() const {
    return Model::Resources::MethodNames::isDeviceItemGroup;
}

bool IsDeviceItemGroupEvent::compare( const ItemEventBase::ItemValue& value ) const {
    return true;
}

}  // namespace SceneEngine
