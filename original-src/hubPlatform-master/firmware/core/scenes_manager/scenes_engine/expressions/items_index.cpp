#include "scenes_manager/scenes_engine/expressions/items_index.h"
#include "scenes_manager/scenes_engine/expressions/resources.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace SceneEngine {
namespace Expressions {

void ItemsIndex::saveItem( const std::string& deviceName, const std::string& itemName, const Json& coreItem ) {
    DeviceItemKey key( deviceName, itemName );
    m_coreItems.emplace( key, coreItem );
    m_itemIdToKey.emplace( json_get_str( coreItem, Resources::Fields::id ), key );
}

bool ItemsIndex::fetchItem( const std::string& itemName, Json& coreItem ) const {
    return fetchDeviceItem( Resources::Values::items, itemName, coreItem );
}

bool ItemsIndex::fetchItemById( const std::string& itemId, Json& coreItem ) const {
    auto deviceItemKey = getItemKeyByItemId( itemId );
    if( deviceItemKey.empty() )
        return false;
    return fetchDeviceItem( deviceItemKey.deviceName(), deviceItemKey.itemName(), coreItem );
}

bool ItemsIndex::fetchDeviceItem( const std::string& deviceName, const std::string& itemName, Json& coreItem ) const {
    DeviceItemKey key = { deviceName, itemName };
    auto foundIt = m_coreItems.find( key );
    if( foundIt != m_coreItems.end() ) {
        coreItem = foundIt->second;
        return true;
    }
    return false;
}

Json ItemsIndex::fillExpressionParams() const {
    Json result = JSON_EMPTY_OBJ;
    for( auto it = m_coreItems.begin(); it != m_coreItems.end(); it++ ) {
        const auto& key = it->first;
        const auto& coreItem = it->second;
        const std::string& variableName = key.itemName();  // should be unique

        // format item value
        const Json& itemValue = coreItem[ Resources::Fields::value ];
        result[ variableName ] = itemValue;
    }
    return result;
}

size_t ItemsIndex::size() const {
    return m_coreItems.size();
}

bool ItemsIndex::empty() const {
    return m_coreItems.empty();
}

ItemsIndex::ItemId ItemsIndex::getItemId( const ItemsIndex::DeviceItemKey& deviceItemKey ) const {
    JsonObject coreItem = JSON_EMPTY_OBJ;
    if( fetchDeviceItem( deviceItemKey.deviceName(), deviceItemKey.itemName(), coreItem ) ) {
        return json_get_str( coreItem, db::ID_FIELD_NAME );
    }
    return ItemId{};
}

ItemsIndex::DeviceItemKey ItemsIndex::getItemKeyByItemId( const ItemId& itemId ) const {
    auto found = m_itemIdToKey.find( itemId );
    if( found != m_itemIdToKey.end() ) {
        return found->second;
    }
    return DeviceItemKey{};
}

void ItemsIndex::fetchAllItemIds( const ItemsIndex::ItemKeysCallback& callback ) const {
    if( callback == nullptr )
        return;
    for( const auto& it : m_itemIdToKey ) {
        callback( it.first );
    }
}

ItemsIndex::DeviceItemKey::DeviceItemKey( std::string deviceName, std::string itemName )
: m_deviceName( std::move( deviceName ) )
, m_itemName( std::move( itemName ) ) {}

bool ItemsIndex::DeviceItemKey::empty() const {
    return m_deviceName.empty() && m_itemName.empty();
}

bool ItemsIndex::DeviceItemKey::addressedByName() const {
    return m_deviceName != Resources::Values::items;
}

const std::string& ItemsIndex::DeviceItemKey::deviceName() const {
    return m_deviceName;
}

const std::string& ItemsIndex::DeviceItemKey::itemName() const {
    return m_itemName;
}

bool ItemsIndex::DeviceItemKey::operator==( const ItemsIndex::DeviceItemKey& r ) const {
    return m_deviceName == r.m_deviceName && m_itemName == r.m_itemName;
}

bool ItemsIndex::DeviceItemKey::operator<( const ItemsIndex::DeviceItemKey& r ) const {
    return m_deviceName < r.m_deviceName || m_itemName < r.m_itemName;
}

}  // namespace Expressions
}  // namespace SceneEngine