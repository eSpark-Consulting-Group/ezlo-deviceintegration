#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {
class CoreDInterface;
}
}  // namespace HubPlatform
class IDbClient;

namespace SceneEngine {
namespace Expressions {

class ItemsIndex final {
public:
    using ItemId = std::string;
    class DeviceItemKey {
    public:
        DeviceItemKey() = default;
        DeviceItemKey( std::string deviceName, std::string itemName );
        bool empty() const;
        bool addressedByName() const;
        const std::string& deviceName() const;
        const std::string& itemName() const;
        bool operator==( const DeviceItemKey& r ) const;
        bool operator<( const DeviceItemKey& r ) const;

    private:
        std::string m_deviceName;
        std::string m_itemName;
    };
    using CoreItem = JsonObject;
    using ItemKeysCallback = std::function< void( const ItemId& itemId ) >;

public:
    bool empty() const;
    size_t size() const;
    void saveItem( const std::string& deviceName, const std::string& itemName, const Json& coreItem );

    bool fetchItem( const std::string& itemName, Json& coreItem ) const;
    bool fetchItemById( const std::string& itemId, Json& coreItem ) const;
    bool fetchDeviceItem( const std::string& deviceName, const std::string& itemName, Json& coreItem ) const;
    Json fillExpressionParams() const;

    ItemId getItemId( const DeviceItemKey& deviceItemKey ) const;
    DeviceItemKey getItemKeyByItemId( const ItemId& itemId ) const;
    void fetchAllItemIds( const ItemsIndex::ItemKeysCallback& callback ) const;

private:
    using KeyToCoreItem = std::map< DeviceItemKey, CoreItem >;
    KeyToCoreItem m_coreItems;
    using ItemIdToKey = std::map< ItemId, DeviceItemKey >;
    ItemIdToKey m_itemIdToKey;
};

}  // namespace Expressions
}  // namespace SceneEngine