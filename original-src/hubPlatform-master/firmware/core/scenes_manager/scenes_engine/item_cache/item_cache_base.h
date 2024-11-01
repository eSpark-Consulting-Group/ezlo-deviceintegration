#pragma once
#include "scenes_manager/scenes_engine/value_source_key.h"
#include "json_utils/json_fwd.h"

namespace SceneEngine {

class ItemCacheBase {
public:
    using Item = JsonObject;

public:
    virtual ~ItemCacheBase() = default;

    virtual void setUp() = 0;
    virtual void tearDown() = 0;
    virtual bool hasKey( const ValueSourceKey& key ) const = 0;
    virtual bool addItem( const ValueSourceKey& key, const Item& item ) = 0;
    virtual bool removeItem( const ValueSourceKey& key ) = 0;
    virtual bool updateItem( const ValueSourceKey& key, const Json& updatedFields ) = 0;
    virtual bool fetchItem( const ValueSourceKey& key, Item& item ) = 0;
};

}  // namespace SceneEngine
