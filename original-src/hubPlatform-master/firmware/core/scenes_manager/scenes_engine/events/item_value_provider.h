#pragma once

#include "scenes_manager/scenes_engine/events/item_value_wrapper.h"

namespace SceneEngine {

class ValueSourceKey;

class ItemValueProviderInterface {
public:
    virtual ~ItemValueProviderInterface() = default;

public:
    virtual bool fetchItemValue() = 0;
    virtual const ItemValueWrapper& getCachedItemValue() const = 0;
    virtual const Json& getCachedError() const = 0;
    virtual bool hasValueKey() const = 0;
    virtual const ValueSourceKey& getValueKey() const = 0;
};
using ItemValueProviderPtr = std::shared_ptr< ItemValueProviderInterface >;

class ItemValueProvider : public ItemValueProviderInterface {
public:
    explicit ItemValueProvider( ItemValueWrapper valueWrapper );

public:
    bool fetchItemValue() override;
    const ItemValueWrapper& getCachedItemValue() const override;
    const Json& getCachedError() const override;
    bool hasValueKey() const override;
    const ValueSourceKey& getValueKey() const override;

private:
    ItemValueWrapper m_itemValue;
};

ItemValueProviderPtr makeItemValueProvider( ItemValueWrapper itemValue );

}  // namespace SceneEngine
