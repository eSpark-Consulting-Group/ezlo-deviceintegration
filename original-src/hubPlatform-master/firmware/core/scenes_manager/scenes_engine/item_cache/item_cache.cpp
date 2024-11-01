#include "scenes_manager/scenes_engine/item_cache/item_cache.h"
#include "database_client/idbclient.h"

namespace SceneEngine {

ItemCache::ItemCache( ControllerFactoryInterface& controllerFactory, IDbClient& dbClient )
: m_controllerFactory( controllerFactory )
, m_dbClient( dbClient ) {}

void ItemCache::setUp() {
    // iterate persistent storage saved keys;
    // fetch all items related from corresponding controllers and fill in-memory cache
    using namespace Resources;
    auto results = m_dbClient.findAll( DbNames::ItemCacheDbName, JSON_EMPTY_OBJ, nullptr );
    for( const auto& result : results ) {
        ValueSourceKey key( "", "" );
        const std::string& stringKey = json_get_str( result, Fields::id );
        if( fromDbKey( stringKey, key ) ) {
            Json fetchedItem;
            if( !fetchItem( key, fetchedItem ) ) {
                LOGGER_ERROR( cc::error( "Failed to fetch item, remove from cache: " ) + cc::note( stringKey ) );
                removeItem( key );
            }
        }
    }
}

void ItemCache::tearDown() {}

bool ItemCache::hasKey( const ValueSourceKey& key ) const {
    return m_items.find( key ) != m_items.end();
}

bool ItemCache::addItem( const ValueSourceKey& key, const Item& item ) {
    // Add item to an in-memory cache;
    // Add item key to a persistent storage
    auto foundIt = m_items.find( key );
    if( m_items.end() == foundIt ) {
        m_dbClient.insert( HaScenesItemCacheDBName, JSON( Resources::Fields::id << toDbKey( key ) ) );
        return m_items.emplace( key, item ).second;
    }
    return false;
}

bool ItemCache::removeItem( const ValueSourceKey& key ) {
    // remove from in-memory cache;
    // Remove from persistent cache
    auto erased = m_items.erase( key );
    m_dbClient.remove( Resources::DbNames::ItemCacheDbName, JSON( Resources::Fields::id << toDbKey( key ) ) );
    return erased > 0;
}

bool ItemCache::updateItem( const ValueSourceKey& key, const Json& updatedFields ) {
    // Update item in in-memory cache
    auto foundIt = m_items.find( key );
    if( foundIt != m_items.end() ) {
        Json newValue = foundIt->second;
        if( !updatedFields.empty() && updatedFields.is_object() ) {
            for( auto it = updatedFields.begin(); it != updatedFields.end(); it++ ) {
                newValue[ it.key() ] = it.value();
            }
            m_items[ key ] = newValue;
            return true;
        }
    }
    return false;
}

bool ItemCache::fetchItem( const ValueSourceKey& key, Item& item ) {
    // fetch in-memory item;
    // if not found, fetch from a corresponding controller and save
    auto foundIt = m_items.find( key );
    if( foundIt != m_items.end() ) {
        item = foundIt->second;
        return true;
    }
    else {
        auto remote = m_controllerFactory.getControllerInterface( key.getControllerId() );
        if( remote ) {
            auto fetchedItem = remote->getItem( key.getSourceId() );
            if( !fetchedItem.empty() ) {
                addItem( key, fetchedItem );
                item = fetchedItem;
                return true;
            }
        }
    }
    return false;
}

std::string toDbKey( const ValueSourceKey& key ) {
    return key.getSourceId() + '.' + key.getControllerId();
}

bool fromDbKey( const std::string& stringKey, ValueSourceKey& key ) {
    auto pos = stringKey.find( '.' );
    if( pos != std::string::npos ) {
        std::string item, controller;
        item = stringKey.substr( 0, pos );
        controller = stringKey.substr( pos + 1, std::string::npos );

        key = ValueSourceKey( controller, item );
        return true;
    }
    return false;
}

}  // namespace SceneEngine
