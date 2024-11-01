#include "scenes_manager/scenes_engine/expressions/expression.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/expressions/resources.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "tiny/tiny_consolecolors.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/remote_interface/cored_interface.h"

namespace SceneEngine {
namespace Expressions {

Expression::Expression( ExpressionManager& manager, std::string expressionName, std::string code, Json params )
: m_manager( manager )
, m_name( std::move( expressionName ) )
, m_code( std::move( code ) )
, m_params( std::move( params ) ) {}

const std::string& Expression::name() const {
    return m_name;
}

const std::string& Expression::code() const {
    return m_code;
}

const Json& Expression::params() const {
    return m_params;
}

const Json& Expression::value() const {
    return m_value;
}

void Expression::value( Json expressionValue ) {
    m_value = std::move( expressionValue );
    m_error.clear();
}

const Json& Expression::error() const {
    return m_error;
}

void Expression::error( Json expressionError ) {
    m_error = std::move( expressionError );
    m_value.clear();
}

const Json& Expression::valueError() const {
    return m_valueError;
}

void Expression::setValueError( Json expressionError ) {
    m_valueError = std::move( expressionError );
}

void Expression::addItemId( Expression::ItemId itemId ) {
    if( !itemId.empty() ) {
        m_items.insert( std::move( itemId ) );
    }
}

void Expression::deleteItemId( const Expression::ItemId& itemId ) {
    if( !itemId.empty() ) {
        m_items.erase( itemId );
    }
}

bool Expression::hasItemId( const Expression::ItemId& itemId ) const {
    return m_items.find( itemId ) != m_items.end();
}

void Expression::clearItemIds() {
    m_items.clear();
}

bool Expression::storeValue( std::string& name, Json& value ) {
    return m_manager.storeValue( m_name, value, {} );
}

bool Expression::storeValueAndError( std::string& name, Json& value, Json& error ) {
    return m_manager.storeValue( m_name, value, error );
}

bool Expression::storeError( std::string& name, Json& error ) {
    return m_manager.storeError( m_name, error );
}

bool Expression::process( Json& answer, Json valueError ) {
    bool ok = false;
    ItemsIndex itemsIndex;
    if( indexItems( answer, itemsIndex ) ) {
        ok = process( answer, itemsIndex, valueError );
    }
    else {
        Json errorReason;
        if( json_get( answer, { "error", "reason" }, errorReason ) && !errorReason.empty() &&
            errorReason.is_string() ) {
            auto errorMessage = json_get_str( errorReason );
            m_manager.storeError( m_name, errorMessage );
            this->error( errorMessage );
        }
    }
    return ok;
}

bool Expression::process( Json& answer, ItemsIndex& itemsIndex, Json& valueError ) {
    using namespace Expressions::Resources;
    bool isOk;
    Json result;
    std::tie( isOk, result ) = ExpressionManager::calculateExpression( answer, m_code, itemsIndex );

    // fetch expression item ids and save it in expressions (expr.name -> itemIds[]) map
    if( !itemsIndex.empty() ) {
        clearItemIds();
        itemsIndex.fetchAllItemIds( [this]( const std::string& itemId ) { addItemId( itemId ); } );
    }

    if( isOk ) {
        LOGGER_DEBUG( cc::debug( "Expression '" ) + name() + cc::debug( "' calculated successfully, value=" ) +
                      cc::j( result ) );
        m_manager.storeValue( m_name, result, m_valueError );
        this->value( std::move( result ) );
        return true;
    }
    else {
        std::string errorMessage = result.is_object() ? json_get_str( result, Fields::error ) : json_get_str( result );
        LOGGER_ERROR( cc::error( "Expression calculation failed for '" + m_name + "', error description: " ) +
                      cc::note( errorMessage ) );
        m_manager.storeError( m_name, errorMessage );
        ::setError( answer, Errors::WRONG_PARAMS, errorMessage );
        this->error( errorMessage );
    }

    return false;
}

bool Expression::indexItems( Json& answer, ItemsIndex& itemsIndex ) {
    auto fetchItemCallback = [&itemsIndex](
                                 const std::string& deviceName, const std::string& itemName, const Json& coreItem ) {
        if( itemName.empty() || coreItem.empty() )
            return false;
        itemsIndex.saveItem( deviceName, itemName, coreItem );
        return true;
    };

    // check params collection: params: { items[], device_item_names[] }
    if( !m_params.empty() ) {
        bool checkOk;
        std::string message;
        std::tie( checkOk, message ) = checkParams( answer, m_params, std::move( fetchItemCallback ) );
        if( !checkOk ) {
            LOGGER_ERROR( cc::error( "Expression parameters checking failed: " ) + cc::note( message ) );
            return false;
        }
    }
    return true;
}

std::pair< bool, std::string > Expression::checkParams( Json& answer,
                                                        const Json& params,
                                                        const FetchItemValue& fetchItemCallback ) {
    // check items
    using namespace Resources;
    std::string errorMessage;
    JsonArray items = JSON_EMPTY_ARR;
    bool hasItems = params.count( Fields::items ) > 0;
    if( hasItems ) {
        if( json_get_arr( params, Fields::items, items ) ) {
            auto codeMessage = checkParamsItems( answer, items, fetchItemCallback );
            if( !codeMessage.first ) {
                return { false, codeMessage.second };
            }
        }
        else {
            // set error: invalid items format
            m_manager.wrongParams( "params.items", answer );
            return { false, "Invalid format of a field: items" };
        }
    }

    // check device_item_names
    bool hasDeviceItemNames = params.count( Fields::deviceItemNames ) > 0;
    if( hasDeviceItemNames ) {
        JsonArray deviceItemNames = JSON_EMPTY_ARR;
        if( json_get_arr( params, Fields::deviceItemNames, deviceItemNames ) ) {
            auto codeMessage = checkParamsDeviceItemsNames( answer, deviceItemNames, fetchItemCallback );
            if( !codeMessage.first )
                return { false, codeMessage.second };
        }
        else {
            // set error: invalid device_item_names format
            m_manager.wrongParams( "params.device_item_names", answer );
            return { false, "Invalid format of a field: device_item_names" };
        }
    }

    return { true, "" };
}

/*
params = {
    "items": [{"name": "kitchenDimmer", "_id": "<itemId>>"}],
    "device_item_names": [{"name": "siren", "deviceName": "Controller Siren", "itemName": "sound_volume" }]
}
 * */
std::pair< bool, std::string > Expression::checkParamsItems( Json& answer,
                                                             const JsonArray& items,
                                                             const FetchItemValue& fetchItemCallback ) {
    using namespace Resources;
    bool hasError = false;
    std::string errorMessage;
    for( auto it = items.begin(); it != items.end(); it++ ) {
        const auto& item = *it;
        if( !item.is_object() ) {
            // set error: invalid format for item position
            hasError = true;
            size_t distance = std::distance( items.begin(), it );
            errorMessage = "params.items[" + std::to_string( distance ) + "]: is not an object";
            m_manager.wrongParams( errorMessage, answer );
            break;
        }
        auto expressionItemName = json_get_str( item, Fields::name );
        auto itemId = json_get_str( item, Fields::id );
        if( expressionItemName.empty() ) {
            // set error: empty field
            hasError = true;
            size_t distance = std::distance( items.begin(), it );
            errorMessage = "params.items[" + std::to_string( distance ) + "]: name is empty";
            m_manager.wrongParams( errorMessage, answer );

            break;
        }
        if( itemId.empty() ) {
            // set error: empty field
            hasError = true;
            size_t distance = std::distance( items.begin(), it );
            errorMessage = "params.items[" + std::to_string( distance ) + "]: _id is empty";
            m_manager.wrongParams( errorMessage, answer );

            break;
        }

        // check item by id
        // somehow this m_core.getItem() call returns previous value of an item, not updated one we see in
        // hub.item.updated broadcast
        JsonObject coreItem = m_manager.fetchCachedItem( itemId, "" );
        if( coreItem.empty() ) {
            hasError = true;
            size_t distance = std::distance( items.begin(), it );
            errorMessage =
                "params.items[" + std::to_string( distance ) + "]: has no item with name='" + expressionItemName + "'";
            setError( answer, Errors::NO_ITEM_FOUND, errorMessage );
            break;
        }

        if( fetchItemCallback != nullptr ) {
            using namespace Expressions::Resources;
            fetchItemCallback( Values::items, expressionItemName, coreItem );
        }
    }
    return { !hasError, errorMessage };
}

std::pair< bool, std::string > Expression::checkParamsDeviceItemsNames( Json& answer,
                                                                        const JsonArray& deviceItemNames,
                                                                        const FetchItemValue& fetchItemCallback ) {
    bool hasError = false;
    std::string errorMessage;
    using namespace Resources;

    for( auto index = 0; index < deviceItemNames.size(); index++ ) {
        const auto& it = deviceItemNames[ index ];
        auto variableName = json_get_str( it, Fields::name );  // name used in expression
        auto deviceName = json_get_str( it, Fields::deviceName );
        auto itemName = json_get_str( it, Fields::itemName );

        if( deviceName.empty() ) {
            hasError = true;
            errorMessage = "device_item_names[ " + std::to_string( index ) + " ].name is empty";
            m_manager.wrongParams( Fields::params, answer );
            return { !hasError, errorMessage };
        }

        auto devices = m_manager.getRemoteInterface().getDevicesBy( JSON( Fields::name << deviceName ) );
        if( devices.empty() ) {
            hasError = true;
            errorMessage =
                "device_item_names[ " + std::to_string( index ) + " ].deviceName; no such device '" + deviceName + "'";
            setError( answer, Errors::NO_DEVICE_FOUND, errorMessage );
            return { !hasError, errorMessage };
        }

        auto deviceId = json_get_str( devices[ 0 ], Fields::id );
        auto items = m_manager.getRemoteInterface().getItemsBy( JSON( "deviceId" << deviceId << "name" << itemName ) );
        if( items.empty() ) {
            hasError = true;
            errorMessage =
                "device_item_names[ " + std::to_string( index ) + " ].itemName: no such item '" + itemName + "'";
            setError( answer, Errors::NO_ITEM_FOUND, errorMessage );
            return { !hasError, errorMessage };
        }

        const auto& itemId = json_get_str( items.at( 0 ), Fields::id );
        // Get item value from cache, not from a remote interface!
        auto cachedItem = m_manager.fetchCachedItem( itemId, "" );
        if( fetchItemCallback != nullptr ) {
            fetchItemCallback( deviceName, variableName, cachedItem );
        }
    }

    return { true, "" };
}

std::set< Expression::ItemId > Expression::getItemIds() const {
    return m_items;
}

}  // namespace Expressions
}  // namespace SceneEngine
