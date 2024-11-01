#pragma once

#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "scenes_manager/scenes_engine/value_source_key.h"

namespace SceneEngine {

class SceneManager;

class GenericItemValueProvider : public ItemValueProviderInterface {
public:
    GenericItemValueProvider( SceneManager& manager, ValueSourceKey key );

public:
    bool fetchItemValue() override;
    const ItemValueWrapper& getCachedItemValue() const override;
    const Json& getCachedError() const override;

    bool hasValueKey() const override;
    const ValueSourceKey& getValueKey() const override;

private:
    SceneManager& m_manager;
    ValueSourceKey m_key;
    ItemValueWrapper m_cachedValue;
    Json m_cachedError;
};

ItemValueProviderPtr makeGenericItemValueProvider( SceneManager& manager, ValueSourceKey key );

}  // namespace SceneEngine
