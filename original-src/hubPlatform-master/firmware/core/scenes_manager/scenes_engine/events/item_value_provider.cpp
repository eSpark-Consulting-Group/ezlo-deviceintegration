#include "scenes_manager/scenes_engine/events/item_value_provider.h"

#include "scenes_manager/scenes_engine/value_source_key.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"

namespace SceneEngine {

ItemValueProvider::ItemValueProvider( ItemValueWrapper valueWrapper )
: m_itemValue( std::move( valueWrapper ) ) {}

bool ItemValueProvider::fetchItemValue() {
    return true;
}

const ItemValueWrapper& ItemValueProvider::getCachedItemValue() const {
    return m_itemValue;
}

const Json& ItemValueProvider::getCachedError() const {
    static Json empty;
    return empty;
}

ItemValueProviderPtr makeItemValueProvider( ItemValueWrapper itemValue ) {
    return std::make_shared< ItemValueProvider >( std::move( itemValue ) );
}

bool ItemValueProvider::hasValueKey() const {
    return false;
}

const ValueSourceKey& ItemValueProvider::getValueKey() const {
    throw EventException( "Unexpected call of \"ItemValueProvider::getValueKey\"" );
}

}  // namespace SceneEngine
