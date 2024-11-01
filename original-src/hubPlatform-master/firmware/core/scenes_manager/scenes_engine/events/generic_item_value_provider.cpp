#include "scenes_manager/scenes_engine/events/generic_item_value_provider.h"

#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "json_utils/json_utils.h"
#include "firmware/core/scenes_manager/modify_scene/resources.h"

namespace SceneEngine {

ItemValueProviderPtr makeGenericItemValueProvider( SceneManager& manager, ValueSourceKey key ) {
    return std::make_shared< GenericItemValueProvider >( manager, std::move( key ) );
}

GenericItemValueProvider::GenericItemValueProvider( SceneManager& manager, ValueSourceKey key )
: m_manager( manager )
, m_key( std::move( key ) ) {}

bool GenericItemValueProvider::fetchItemValue() {
    try {
        m_cachedValue = m_manager.fetchSourceValue( m_key );
        m_cachedError = {};
        return true;
    }
    catch( const SceneException& e ) {
        m_cachedValue = ItemValueWrapper();
        m_cachedError = "Failed to get value for: " + m_key.getSourceId() + " : " + std::string( e.what() );
        return false;
    }
    catch( const std::exception& e ) {
        m_cachedValue = ItemValueWrapper();
        m_cachedError = "Failed to get value for: " + m_key.getSourceId() + " : " + std::string( e.what() );
        return false;
    }
    catch( ... ) {
        m_cachedValue = ItemValueWrapper();
        m_cachedError = "Exception while getting value for: " + m_key.getSourceId();
        return false;
    }
}

const ItemValueWrapper& GenericItemValueProvider::getCachedItemValue() const {
    return m_cachedValue;
}

const Json& GenericItemValueProvider::getCachedError() const {
    return m_cachedError;
}

bool GenericItemValueProvider::hasValueKey() const {
    return true;
}

const ValueSourceKey& GenericItemValueProvider::getValueKey() const {
    return m_key;
}

}  // namespace SceneEngine
