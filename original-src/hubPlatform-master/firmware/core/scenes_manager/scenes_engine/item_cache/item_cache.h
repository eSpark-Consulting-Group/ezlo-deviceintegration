#pragma once
#include "scenes_manager/scenes_engine/item_cache/item_cache_base.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache_resources.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"

class IDbClient;

namespace SceneEngine {

class ItemCache : public ItemCacheBase {
public:
    ~ItemCache() override = default;
    ItemCache( ControllerFactoryInterface& controllerFactory, IDbClient& dbClient );

    void setUp() override;
    void tearDown() override;
    bool hasKey( const ValueSourceKey& key ) const override;
    bool addItem( const ValueSourceKey& key, const Item& item ) override;
    bool removeItem( const ValueSourceKey& key ) override;
    bool updateItem( const ValueSourceKey& key, const Json& updatedFields ) override;
    bool fetchItem( const ValueSourceKey& key, Item& item ) override;

private:
    ControllerFactoryInterface& m_controllerFactory;
    IDbClient& m_dbClient;
    using ItemsMap = std::map< ValueSourceKey, Item >;
    ItemsMap m_items;
};

std::string toDbKey( const ValueSourceKey& key );
bool fromDbKey( const std::string& stringKey, ValueSourceKey& key );

}  // namespace SceneEngine