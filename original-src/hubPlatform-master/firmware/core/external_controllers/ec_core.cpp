#include "external_controllers/ec_core.h"
#include "external_controllers/resources/ec_resources.h"
#include "external_controllers/common/ec_common_utils.h"

#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace ExternalControllers {

Core::Core( Messenger& messenger, IDbClient& dbClient )
: m_messenger( messenger )
, m_dbClient( dbClient ) {
    auto fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 );
    auto subscriptions = m_dbClient.findAll( Resources::DBTableNames::Subscriptions, JSON_EMPTY_OBJ, &fieldsToReturn );
    for( auto& subscription : subscriptions ) {
        auto controllerId = json_get_str( subscription, db::ID_FIELD_NAME );
        syncControllerData( controllerId );
    }
}

void Core::subscribe( const std::string& controllerId,
                      std::vector< std::string > itemsIds,
                      std::vector< std::string > devicesIds ) {
    using namespace Resources;
    Log::debug( cc::debug( "Subscribing on controller " ) + cc::note( controllerId ) + cc::debug( " entries" ) );
    // TODO: maintain separate subscriptions for each service

    std::sort( itemsIds.begin(), itemsIds.end() );
    std::sort( devicesIds.begin(), devicesIds.end() );

    auto query = JSON( db::ID_FIELD_NAME << controllerId );
    auto subscriptions = m_dbClient.findOne( DBTableNames::Subscriptions, query, nullptr );
    if( subscriptions.empty() ) {
        subscriptions[ db::ID_FIELD_NAME ] = controllerId;
        subscriptions[ "itemsIds" ] = itemsIds;
        subscriptions[ "devicesIds" ] = devicesIds;

        m_dbClient.insert( DBTableNames::Subscriptions, subscriptions );
        m_messenger.registerControllerConnection( controllerId );
    }
    else {
        subscriptions[ db::ID_FIELD_NAME ] = controllerId;
        auto currentItemsIds = json_get_vec< std::string >( subscriptions, "itemsIds" );
        auto currentDevicesIds = json_get_vec< std::string >( subscriptions, "devicesIds" );

        std::vector< std::string > newItemsIds;
        std::vector< std::string > newDevicesIds;

        std::set_union( currentItemsIds.begin(),
                        currentItemsIds.end(),
                        itemsIds.begin(),
                        itemsIds.end(),
                        std::back_inserter( newItemsIds ) );
        std::set_union( currentDevicesIds.begin(),
                        currentDevicesIds.end(),
                        devicesIds.begin(),
                        devicesIds.end(),
                        std::back_inserter( newDevicesIds ) );

        subscriptions[ "itemsIds" ] = newItemsIds;
        subscriptions[ "devicesIds" ] = newDevicesIds;

        m_dbClient.update( DBTableNames::Subscriptions, query, subscriptions );
    }

    if( !itemsIds.empty() ) {
        m_messenger.requestItemsList( controllerId );
    }
    if( !devicesIds.empty() ) {
        m_messenger.requestDevicesList( controllerId );
    }
}

void Core::unsubscribe( const std::string& controllerId,
                        std::vector< std::string > itemsIds,
                        std::vector< std::string > devicesIds ) {
    Log::debug( cc::debug( "Unsubscribing from controller " ) + cc::note( controllerId ) + cc::debug( " entries" ) );
    using namespace Resources;
    std::sort( itemsIds.begin(), itemsIds.end() );
    std::sort( devicesIds.begin(), devicesIds.end() );

    auto query = JSON( db::ID_FIELD_NAME << controllerId );
    auto subscriptions = m_dbClient.findOne( DBTableNames::Subscriptions, query, nullptr );
    if( subscriptions.empty() ) {
        Log::warn( cc::warn( "Trying to unsubscribe from controller that is not subscribed" ) );
        return;
    }

    subscriptions[ db::ID_FIELD_NAME ] = controllerId;
    auto currentItemsIds = json_get_vec< std::string >( subscriptions, "itemsIds" );
    auto currentDevicesIds = json_get_vec< std::string >( subscriptions, "devicesIds" );

    std::vector< std::string > newItemsIds;
    std::vector< std::string > newDevicesIds;

    std::set_difference( currentItemsIds.begin(),
                         currentItemsIds.end(),
                         itemsIds.begin(),
                         itemsIds.end(),
                         std::back_inserter( newItemsIds ) );
    std::set_difference( currentDevicesIds.begin(),
                         currentDevicesIds.end(),
                         devicesIds.begin(),
                         devicesIds.end(),
                         std::back_inserter( newDevicesIds ) );

    if( newItemsIds.empty() && newDevicesIds.empty() ) {
        m_dbClient.remove( DBTableNames::Subscriptions, query );
        m_messenger.unregisterControllerConnection( controllerId );
    }
    else {
        subscriptions[ "itemsIds" ] = newItemsIds;
        subscriptions[ "devicesIds" ] = newDevicesIds;

        m_dbClient.update( DBTableNames::Subscriptions, query, subscriptions );
    }

    // Remove items and devices without sending notifications
    if( !itemsIds.empty() ) {
        std::vector< std::string > ids;
        std::transform( itemsIds.begin(), itemsIds.end(), std::back_inserter( ids ), [&]( const std::string& itemId ) {
            return Utils::generateDbId( controllerId, itemId );
        } );
        m_dbClient.remove( DBTableNames::Items, JSON( db::ID_FIELD_NAME << JSON( "$in" << ids ) ) );
    }
    if( !devicesIds.empty() ) {
        std::vector< std::string > ids;
        std::transform(
            devicesIds.begin(), devicesIds.end(), std::back_inserter( ids ), [&]( const std::string& deviceId ) {
                return Utils::generateDbId( controllerId, deviceId );
            } );
        m_dbClient.remove( DBTableNames::Devices, JSON( db::ID_FIELD_NAME << JSON( "$in" << ids ) ) );
    }
}

void Core::syncControllerData( const std::string& controllerId ) {
    auto subscription = m_dbClient.findOne(
        Resources::DBTableNames::Subscriptions, JSON( db::ID_FIELD_NAME << controllerId ), nullptr );
    auto itemsIds = json_get_vec< std::string >( subscription, "itemsIds" );
    if( !itemsIds.empty() ) {
        m_messenger.requestItemsList( controllerId );
    }
    auto devicesIds = json_get_vec< std::string >( subscription, "devicesIds" );
    if( !devicesIds.empty() ) {
        m_messenger.requestDevicesList( controllerId );
    }
}

void Core::updateItemsList( const std::string& controllerId, const JsonArray& items ) {
    using namespace Resources;
    // Get subscribed items list
    auto subscriptions =
        m_dbClient.findOne( DBTableNames::Subscriptions, JSON( db::ID_FIELD_NAME << controllerId ), nullptr );
    if( subscriptions.empty() ) {
        Log::warn( cc::warn( "Got items list for unsubscribed controller" ) );
        return;
    }

    auto subscribedItemsIds = json_get_vec< std::string >( subscriptions, "itemsIds" );
    std::set< std::string > subscribedItemsIdsSet( subscribedItemsIds.begin(), subscribedItemsIds.end() );

    // Get current devices list
    std::set< std::string > currentItemsIds;
    auto fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 );
    auto currentItems = m_dbClient.findAll( DBTableNames::Items, JSON_EMPTY_OBJ, &fieldsToReturn );
    for( auto& itemJson : currentItems ) {
        auto id = json_get_str( itemJson, db::ID_FIELD_NAME );
        auto currentControllerId = Utils::extractControllerId( id );
        if( currentControllerId == controllerId ) {
            auto currentItemId = Utils::extractId( id );
            currentItemsIds.insert( std::move( currentItemId ) );
        }
    }

    std::map< std::string, int > newItemsIndexesByIds;
    for( int i = 0; i < items.size(); ++i ) {
        auto& item = items[ i ];
        auto itemId = json_get_str( item, db::ID_FIELD_NAME );
        if( subscribedItemsIdsSet.count( itemId ) != 0 ) {
            newItemsIndexesByIds.insert( std::make_pair( std::move( itemId ), i ) );
        }
    }

    for( const auto& itemId : currentItemsIds ) {
        if( newItemsIndexesByIds.count( itemId ) == 0 ) {
            removeItem( controllerId, itemId );
        }
    }

    for( const auto& itemIdWithIndex : newItemsIndexesByIds ) {
        auto& itemId = itemIdWithIndex.first;
        auto& itemIndex = itemIdWithIndex.second;
        if( currentItemsIds.count( itemId ) == 0 ) {
            addItem( controllerId, itemId, items[ itemIndex ] );
        }
        else {
            Json itemDiff = getItemDiff( controllerId, itemId, items[ itemIndex ] );
            if( !itemDiff.empty() ) {
                updateItem( controllerId, itemId, itemDiff );
            }
        }
    }
}

void Core::updateDevicesList( const std::string& controllerId, const JsonArray& devices ) {
    using namespace Resources;
    // Get subscribed devices list
    auto subscriptions =
        m_dbClient.findOne( DBTableNames::Subscriptions, JSON( db::ID_FIELD_NAME << controllerId ), nullptr );
    if( subscriptions.empty() ) {
        Log::warn( cc::warn( "Got devices list for unsubscribed controller" ) );
        return;
    }

    auto subscribedDevicesIds = json_get_vec< std::string >( subscriptions, "devicesIds" );
    std::set< std::string > subscribedDevicesIdsSet( subscribedDevicesIds.begin(), subscribedDevicesIds.end() );

    // Get current devices list
    std::set< std::string > currentDevicesIds;
    auto fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 );
    auto currentDevices = m_dbClient.findAll( DBTableNames::Devices, JSON_EMPTY_OBJ, &fieldsToReturn );
    for( auto& deviceJson : currentDevices ) {
        auto id = json_get_str( deviceJson, db::ID_FIELD_NAME );
        auto currentControllerId = Utils::extractControllerId( id );
        if( currentControllerId == controllerId ) {
            auto currentDeviceId = Utils::extractId( id );
            currentDevicesIds.insert( std::move( currentDeviceId ) );
        }
    }

    std::map< std::string, int > newDevicesIndexesByIds;
    for( int i = 0; i < devices.size(); ++i ) {
        auto& device = devices[ i ];
        auto deviceId = json_get_str( device, db::ID_FIELD_NAME );
        if( subscribedDevicesIdsSet.count( deviceId ) != 0 ) {
            newDevicesIndexesByIds.insert( std::make_pair( std::move( deviceId ), i ) );
        }
    }

    for( const auto& deviceId : currentDevicesIds ) {
        if( newDevicesIndexesByIds.count( deviceId ) == 0 ) {
            removeDevice( controllerId, deviceId );
        }
    }

    for( const auto& deviceIdWithIndex : newDevicesIndexesByIds ) {
        auto& deviceId = deviceIdWithIndex.first;
        auto& deviceIndex = deviceIdWithIndex.second;
        if( currentDevicesIds.count( deviceId ) == 0 ) {
            addDevice( controllerId, deviceId, devices[ deviceIndex ] );
        }
        else {
            Json deviceDiff = getDeviceDiff( controllerId, deviceId, devices[ deviceIndex ] );
            if( !deviceDiff.empty() ) {
                updateDevice( controllerId, deviceId, deviceDiff );
            }
        }
    }
}

Json Core::getItemDiff( const std::string& controllerId, const std::string& itemId, const Json& newItem ) const {
    using namespace Resources;
    auto currentDevice = m_dbClient.findOne(
        DBTableNames::Items, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ), nullptr );

    auto diff = jsonDiff( currentDevice, newItem, true );
    json_remove( diff, db::ID_FIELD_NAME );
    return diff;
}

JsonObject Core::getItem( const std::string& controllerId, const std::string& itemId ) const {
    using namespace Resources;
    return m_dbClient.findOne(
        DBTableNames::Items, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ), nullptr );
}

bool Core::itemSubscribed( const std::string& controllerId, const std::string& itemId ) const {
    auto subscription = m_dbClient.findOne(
        Resources::DBTableNames::Subscriptions,
        JSON( db::ID_FIELD_NAME << controllerId << "itemsIds" << JSON( "$in" << JSON_ARRAY_( itemId ) ) ),
        nullptr );
    return !subscription.empty();
}

bool Core::itemExists( const std::string& controllerId, const std::string& itemId ) const {
    using namespace Resources;
    return m_dbClient.has( DBTableNames::Items,
                           JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ) );
}

void Core::addItem( const std::string& controllerId, const std::string& itemId, Json item ) {
    using namespace Resources;
    Log::debug( cc::debug( "Adding controller " ) + cc::note( controllerId ) + cc::debug( " item with id " ) +
                cc::note( itemId ) );

    item[ db::ID_FIELD_NAME ] = Utils::generateDbId( controllerId, itemId );
    m_dbClient.insert( DBTableNames::Items, item );

    item[ db::ID_FIELD_NAME ] = itemId;
    item[ "controllerId" ] = controllerId;
    m_messenger.sendInternalBroadcast( HUB_ITEM_ADDED, item );
}

void Core::updateItem( const std::string& controllerId, const std::string& itemId, const Json& item ) {
    using namespace Resources;

    Json query = item;
    query[ db::ID_FIELD_NAME ] = Utils::generateDbId( controllerId, itemId );
    bool nothingChanged = m_dbClient.has( DBTableNames::Items, query );
    if( nothingChanged ) {
        Log::debug( cc::debug( "Unchanged controller " ) + cc::note( controllerId ) + cc::debug( " item with id " ) +
                    cc::note( itemId ) );
        return;
    }

    Log::debug( cc::debug( "Updating controller " ) + cc::note( controllerId ) + cc::debug( " item with id " ) +
                cc::note( itemId ) );

    auto originalItem = m_dbClient.findOne(
        DBTableNames::Items, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ), nullptr );

    m_dbClient.update( DBTableNames::Items,
                       JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ),
                       JSON( "$set" << item ) );

    if( originalItem.empty() ) {
        Log::error( cc::error( "Items integrity check failed" ) );
        return;
    }

    if( item.count( "value" ) != 0 && json_get_str( originalItem, "valueType" ) == "dictionary" ) {
        Json data;
        data[ db::ID_FIELD_NAME ] = itemId;
        data[ "controllerId" ] = controllerId;
        data[ "deviceId" ] = originalItem[ "deviceId" ];
        data[ "name" ] = originalItem[ "name" ];

        const Json& oldValue = originalItem[ "value" ];
        const Json& newValue = item[ "value" ];

        for( auto it = oldValue.begin(); it != oldValue.end(); ++it ) {
            if( newValue.count( it.key() ) == 0 ) {
                data[ "operation" ] = "removed";
                data[ "element" ] = JSON( it.key() << JSON( "value" << it.value() ) );
                m_messenger.sendInternalBroadcast( HUB_ITEM_DICTIONARY_UPDATED, data );
            }
        }

        for( auto it = newValue.begin(); it != newValue.end(); ++it ) {
            if( oldValue.count( it.key() ) == 0 ) {
                data[ "operation" ] = "added";
                data[ "element" ] = JSON( it.key() << JSON( "value" << it.value() ) );
                m_messenger.sendInternalBroadcast( HUB_ITEM_DICTIONARY_UPDATED, data );
            }
            else if( it.value() != oldValue[ it.key() ] ) {
                data[ "operation" ] = "updated";
                data[ "element" ] = JSON( it.key() << JSON( "value" << it.value() ) );
                m_messenger.sendInternalBroadcast( HUB_ITEM_DICTIONARY_UPDATED, data );
            }
        }
    }
    else {
        Json data = item;
        data[ db::ID_FIELD_NAME ] = itemId;
        data[ "controllerId" ] = controllerId;
        data[ "deviceId" ] = originalItem[ "deviceId" ];
        data[ "name" ] = originalItem[ "name" ];
        m_messenger.sendInternalBroadcast( HUB_ITEM_UPDATED, data );
    }
}

void Core::removeItem( const std::string& controllerId, const std::string& itemId ) {
    using namespace Resources;
    Log::debug( cc::debug( "Removing controller " ) + cc::note( controllerId ) + cc::debug( " item with id " ) +
                cc::note( itemId ) );

    m_dbClient.remove( DBTableNames::Items, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ) );

    Json data;
    data[ db::ID_FIELD_NAME ] = itemId;
    data[ "controllerId" ] = controllerId;
    m_messenger.sendInternalBroadcast( HUB_ITEM_REMOVED, data );
}

Json Core::getDeviceDiff( const std::string& controllerId, const std::string& deviceId, const Json& newDevice ) const {
    using namespace Resources;
    auto currentDevice = m_dbClient.findOne(
        DBTableNames::Devices, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, deviceId ) ), nullptr );

    auto diff = jsonDiff( currentDevice, newDevice, true );
    json_remove( diff, db::ID_FIELD_NAME );
    return diff;
}

bool Core::deviceSubscribed( const std::string& controllerId, const std::string& deviceId ) const {
    auto subscription = m_dbClient.findOne(
        Resources::DBTableNames::Subscriptions,
        JSON( db::ID_FIELD_NAME << controllerId << "devicesIds" << JSON( "$in" << JSON_ARRAY_( deviceId ) ) ),
        nullptr );
    return !subscription.empty();
}

bool Core::deviceExists( const std::string& controllerId, const std::string& deviceId ) const {
    using namespace Resources;
    return m_dbClient.has( DBTableNames::Devices,
                           JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, deviceId ) ) );
}

void Core::addDevice( const std::string& controllerId, const std::string& deviceId, Json device ) {
    using namespace Resources;
    Log::debug( cc::debug( "Adding controller " ) + cc::note( controllerId ) + cc::debug( " device with id " ) +
                cc::note( deviceId ) );

    device[ db::ID_FIELD_NAME ] = Utils::generateDbId( controllerId, deviceId );
    m_dbClient.insert( DBTableNames::Devices, device );

    device[ db::ID_FIELD_NAME ] = deviceId;
    device[ "controllerId" ] = controllerId;
    m_messenger.sendInternalBroadcast( HUB_DEVICE_ADDED, device );
}

void Core::updateDevice( const std::string& controllerId, const std::string& deviceId, const Json& device ) {
    using namespace Resources;

    Json query = device;
    query[ db::ID_FIELD_NAME ] = Utils::generateDbId( controllerId, deviceId );
    bool nothingChanged = m_dbClient.has( DBTableNames::Devices, query );
    if( nothingChanged ) {
        Log::debug( cc::debug( "Unchanged controller " ) + cc::note( controllerId ) + cc::debug( " device with id " ) +
                    cc::note( deviceId ) );
        return;
    }

    Log::debug( cc::debug( "Updating controller " ) + cc::note( controllerId ) + cc::debug( " device with id " ) +
                cc::note( deviceId ) );

    m_dbClient.update( DBTableNames::Devices,
                       JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, deviceId ) ),
                       JSON( "$set" << device ) );

    Json data = device;
    data[ db::ID_FIELD_NAME ] = deviceId;
    data[ "controllerId" ] = controllerId;
    m_messenger.sendInternalBroadcast( HUB_DEVICE_UPDATED, data );
}

void Core::removeDevice( const std::string& controllerId, const std::string& deviceId ) {
    using namespace Resources;
    Log::debug( cc::debug( "Removing controller " ) + cc::note( controllerId ) + cc::debug( " device with id " ) +
                cc::note( deviceId ) );

    m_dbClient.remove( DBTableNames::Devices,
                       JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, deviceId ) ) );

    Json data;
    data[ db::ID_FIELD_NAME ] = deviceId;
    data[ "controllerId" ] = controllerId;
    m_messenger.sendInternalBroadcast( HUB_DEVICE_REMOVED, data );
}

}  // namespace ExternalControllers
}  // namespace HubPlatform
