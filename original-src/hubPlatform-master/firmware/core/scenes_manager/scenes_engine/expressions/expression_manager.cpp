#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/expressions/items_index.h"
#include "scenes_manager/scenes_engine/expressions/resources.h"
#include "scenes_manager/scenes_engine/value_source_key.h"

#include "scenes_manager/lua_interpreter/interpreter.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/remote_interface/cored_interface.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"

namespace SceneEngine {
namespace Expressions {

namespace {
void wrongParam( const std::string& fieldName, JsonObject& answer ) {
    JsonObject error_mes;
    Errors::getWrongParams( fieldName, error_mes );
    setErrorObject( answer, error_mes );
}
}  // namespace

ExpressionManager::ExpressionManager( HubPlatform::CoreDRemoteInterface::CoreDInterface& coreRemote,
                                      IDbClient& dbClient,
                                      SceneEngine::ItemCacheBase& itemCache,
                                      std::string controllerId )
: m_ownControllerId( std::move( controllerId ) )
, m_core( coreRemote )
, m_dbClient( dbClient )
, m_itemCache( itemCache ) {}

void ExpressionManager::setUp() {
    auto records = m_dbClient.findAll( HaScenesExpressionsDBName, JSON_EMPTY_OBJ, nullptr );
    if( !records.empty() ) {
        LOGGER_INFO( cc::info( "Re-evaluate all expressions: " ) + cc::num10( records.size() ) );
    }

    for( const auto& record : records ) {
        using namespace Expressions::Resources;

        std::string name = json_get_str( record, Fields::id );
        std::string code = json_get_str( record, Fields::code );
        std::string typeName = json_get_str( record, Fields::valueType );
        Json value;
        bool hasValue = json_get( record, Fields::value, value );
        Json error;
        bool hasError = json_get( record, Fields::error, error );
        Json metadata;
        json_get( record, Fields::metadata, metadata );
        bool variable = json_get_bool( record, Fields::variable );
        Json answer, params;
        json_get( record, Fields::params, params );
        if( variable && hasValue && code.empty() ) {
            TypedVariableExpressionUPtr expressionUPtr(
                new TypedVariableExpression( *this, name, value, params, typeName, metadata, variable, error ) );

            expressionUPtr->value( std::move( value ) );

            auto addedIt = m_valueExpressions.emplace( name, std::move( expressionUPtr ) );

            TypedVariableExpressionUPtr& expressionPtr = addedIt.first->second;
            expressionPtr->storeValueAndError( name, value, error );
            updateItemEntries( name, *expressionPtr );
        }
        else {
            TypedExpressionUPtr expressionUPtr(
                new TypedExpression( *this, name, code, params, typeName, metadata, variable ) );
            using namespace HubPlatform::CoreD;

            JsonArrayBuilder valueErrorArr;

            bool isOk;
            Json result;
            SceneEngine::Expressions::ItemsIndex itemsIndex;
            if( expressionUPtr->indexItems( answer, itemsIndex ) ) {
                std::tie( isOk, result ) = calculateExpression( answer, code, itemsIndex );
                if( isOk ) {
                    auto valueTypeEnum = Model::ValueType::fromString( expressionUPtr->valueType() );
                    try {
                        Model::ValueFactory::createItemValue( valueTypeEnum, result );
                    }
                    catch( const HubPlatform::CoreD::Model::UnexpectedValueType& ex ) {
                        valueErrorArr << ex.what();
                        LOGGER_ERROR( cc::error( "Unexpected value type: : " ).append( ex.what() ) );
                    }
                    catch( const std::exception& ex ) {
                        valueErrorArr << ex.what();
                        LOGGER_ERROR( cc::error( "Exception: " ).append( ex.what() ) );
                    }
                    catch( ... ) {
                        const auto errorMessage = "Create item value internal error";
                        valueErrorArr << errorMessage;
                        LOGGER_ERROR( cc::error( "Exception: " ).append( errorMessage ) );
                    }

                    if( variable && !params.empty() ) {
                        const auto errorMessage = "Variable state does not match params value";
                        valueErrorArr << errorMessage;
                        LOGGER_ERROR( cc::error( "Exception: " ).append( errorMessage ) );
                    }
                }
            }
            else {
                const auto errorMessage = result.is_object() ? json_get_str( result, "error" ) : json_get_str( result );
                valueErrorArr << errorMessage;
                LOGGER_ERROR( cc::error( errorMessage ) );
            }

            auto addedIt = m_expressions.emplace( name, std::move( expressionUPtr ) );

            try {
                LOGGER_DEBUG( cc::debug( "Re-calculate expression " ) + cc::note( name ) );
                if( addedIt.second ) {
                    TypedExpressionUPtr& expressionPtr = addedIt.first->second;
                    if( variable ) {
                        expressionPtr->storeValue( name, value );
                        expressionPtr->storeError( name, error );
                    }
                    expressionPtr->process( answer, valueErrorArr );
                    updateItemEntries( name, *expressionPtr );
                }
            }
            catch( const std::exception& ex ) {
                Log::error( cc::error( "ExpressionManager::setUp: exception precessing an expression: " ) +
                            cc::note( ex.what() ) );
            }
        }
    }
}

void ExpressionManager::tearDown() {
    m_valueChangedCallback = nullptr;
    m_expressionsWithItem.clear();
    m_expressions.clear();
    m_valueExpressions.clear();
}

Json ExpressionManager::findByName( const std::string& name ) const {
    auto prevValue = m_dbClient.findOne( HaScenesExpressionsDBName, JSON( Resources::Fields::id << name ), nullptr );
    return prevValue;
}

bool ExpressionManager::insertExpression( const std::string& name,
                                          const std::string& code,
                                          const Json& params,
                                          const std::string& valueType,
                                          const Json& metadata,
                                          const Json& value,
                                          bool variable,
                                          const Json& error ) {
    if( name.empty() || ( !variable && code.empty() ) || ( variable && value.is_null() ) ) {
        return false;
    }

    using namespace Expressions::Resources;
    m_dbClient.insert( HaScenesExpressionsDBName,
                       JSON( Fields::id << name << Fields::code << code << Fields::params << params << Fields::valueType
                                        << valueType << Fields::metadata << metadata << Fields::variable << variable
                                        << Fields::value << value << Fields::error << error ) );
    m_dbClient.remove( HaTempScenesExpressionValuesDBName, JSON( Fields::id << name ) );
    return true;
}

bool ExpressionManager::deleteExpression( const std::string& name ) {
    if( name.empty() )
        return false;
    m_dbClient.remove( HaScenesExpressionsDBName, JSON( Resources::Fields::id << name ) );
    m_dbClient.remove( HaTempScenesExpressionValuesDBName, JSON( Resources::Fields::id << name ) );
    return true;
}

/**  Calls Lua interpreter and fills request Lua script to evaluate in it */
std::pair< bool, Json > ExpressionManager::calculateExpression( Json& answer,
                                                                const std::string& code,
                                                                const ItemsIndex& itemsIndex ) {
    HubPlatform::ScenesExpressions::ScenesLuaInterpreter luaInterpreter;
    std::string scriptText = "local params=...\n" + code;
    JsonObject expressionParams = itemsIndex.fillExpressionParams();
    Json result;
    if( luaInterpreter.protectedCall( scriptText, expressionParams, result ) ) {
        return { true, result };
    }

    // error, result: { error: string }
    return { false, result };
}

bool ExpressionManager::storeField( const std::string& name,
                                    const std::string& field,
                                    const Json& value,
                                    const Json& error ) {
    if( name.empty() || field.empty() )
        return false;

    const bool upsert = true;
    JsonObject record;
    if( !error.empty() ) {
        record = JSON( Resources::Fields::id << name << field << value << Resources::Fields::valueError << error );
    }
    else {
        record = JSON( Resources::Fields::id << name << field << value );
    }

    m_dbClient.update( HaTempScenesExpressionValuesDBName, JSON_EMPTY_OBJ, record, upsert );

    return true;
}

bool ExpressionManager::storeValue( const std::string& name, const Json& value, const Json& error ) {
    return storeField( name, Resources::Fields::value, value, error );
}

bool ExpressionManager::storeError( const std::string& name, const Json& error ) {
    return storeField( name, Resources::Fields::error, error, JSON_EMPTY_OBJ );
}

ExpressionManager::FetchResult::FetchResult( Status status, Json error )
: status( status )
, error( std::move( error ) )
, hasValue( false ) {}

ExpressionManager::FetchResult::FetchResult( Status status, Json error, Json value )
: status( status )
, error( std::move( error ) )
, value( std::move( value ) )
, hasValue( true ) {}

/** @brief Returns fetch result for expression
 * NOTE: Expression value will be returned if found, even if errors are present
 * @param name - expression name
 * @return FetchResult - struct with error and/or expression value
 */
ExpressionManager::FetchResult ExpressionManager::fetchValue( const std::string& name ) const {
    using namespace Expressions::Resources;
    using Status = FetchResult::Status;

    auto result = m_dbClient.findOne( HaTempScenesExpressionValuesDBName, JSON( Fields::id << name ), nullptr );
    if( !result.empty() ) {
        Json expressionError;
        if( json_get( result, Fields::error, expressionError ) )
            return { Status::Error, std::move( expressionError ) };

        Json expressionValue;
        const bool valueIsFound = json_get( result, Fields::value, expressionValue );

        if( json_get( result, Fields::valueError, expressionError ) ) {
            if( valueIsFound )
                return { Status::ValueError, std::move( expressionError ), std::move( expressionValue ) };
            return { Status::ValueError, std::move( expressionError ) };
        }

        if( !valueIsFound )
            return { Status::ValueNotFound, "Expression value not found" };

        return { Status::Ok, {}, std::move( expressionValue ) };
    }

    return { Status::NotFound, "Expression not found" };
}

std::string ExpressionManager::findValueType( const std::string& name ) {
    auto expression = findByName( name );
    return json_get_str( expression, Resources::Fields::valueType );
}

bool ExpressionManager::hasExpression( const std::string& name ) const {
    return m_dbClient.has( HaScenesExpressionsDBName, JSON( Resources::Fields::id << name ) );
}

std::vector< Json > ExpressionManager::findExpressionsBy( const Json& query ) const {
    return m_dbClient.findAll( HaScenesExpressionsDBName, query, nullptr );
}

void ExpressionManager::updateCachedItemFromBroadcast( const ValueSourceKey& searchKey, const Json& message ) {
    Json convertedValue = JSON_EMPTY_OBJ;
    if( extractValueFromBroadcast( message, convertedValue ) ) {
        LOGGER_TRACE( "updateItem with converted value: " + cc::j( convertedValue ) );
        m_itemCache.updateItem( searchKey, convertedValue );
    }
    else {
        m_itemCache.updateItem( searchKey, message );
    }
}

/** @brief Transforms a broadcast data plain object for scalable value type to nested values object:
 *  @params [in] broadcastData - data of a item.updated broadcast;
 *  @params [out] convertedValue - fields with converted "value" field.
 *  @example Example:
 *          { ..., "value: someValue, "scale": "someScale" } ->
 *          { ..., "value": { "value": someValue, "scale": "someScale" } }
 *  @returns true - data transformed and the result is placed to convertedValue;
 *  @returns false - no conversion was made, convertedValue is not changed.
 * */
bool ExpressionManager::extractValueFromBroadcast( const Json& broadcastData, Json& convertedValue ) {
    if( broadcastData.count( "value" ) ) {
        if( broadcastData.count( "scale" ) ) {
            Json updatedFields = JSON_EMPTY_OBJ, compoundValue;
            updatedFields[ "value" ] =
                JSON( "value" << broadcastData[ "value" ] << "scale" << broadcastData[ "scale" ] );

            if( !broadcastData.empty() && broadcastData.is_object() ) {
                for( auto it = broadcastData.begin(); it != broadcastData.end(); it++ ) {
                    const auto& key = it.key();
                    if( key != "value" && key != "scale" ) {
                        updatedFields[ key ] = it.value();
                    }
                }
                convertedValue = std::move( updatedFields );
                return true;
            }
        }
    }
    else {
        LOGGER_TRACE( "has no value at all" );
    }
    return false;
}

bool ExpressionManager::handleItemUpdated( const Json& message ) {
    using namespace Expressions::Resources;

    auto itemId = json_get_str( message, Fields::id );
    SceneEngine::ValueSourceKey searchKey = makeSearchKey( itemId );
    bool isInCache = m_itemCache.hasKey( searchKey );

    auto found = m_expressionsWithItem.find( itemId );
    if( found == m_expressionsWithItem.end() ) {
        if( isInCache ) {
            m_itemCache.updateItem( searchKey, message );
        }

        return false;
    }
    ItemEntry& itemEntry = found->second;

    // Check if item name have changed: move iteId from expression's set of items, mark an expression as ill-formed.
    auto itemName = json_get_str( message, Fields::name );
    Json itemValue;
    bool isItemNameChanged = itemEntry.name() != itemName;

    if( isItemNameChanged ) {
        LOGGER_TRACE( cc::trace( "Item was renamed: '" ) + itemEntry.name() + cc::trace( "' -> '" ) + itemName +
                      cc::trace( "'" ) );
        itemEntry.name( itemName );
    }

    Json coreItem = JSON_EMPTY_OBJ;
    bool isItemFetched = m_itemCache.fetchItem( searchKey, coreItem );

    Json convertedValue = JSON_EMPTY_OBJ;
    bool hasItemValue = json_get( message, Fields::value, itemValue );
    if( !extractValueFromBroadcast( message, itemValue ) )
        itemValue = message;

    bool isItemValueChanged = isItemFetched && hasItemValue && coreItem[ Fields::value ] != itemValue[ Fields::value ];
    if( isItemValueChanged ) {
        LOGGER_TRACE( cc::trace( "Item value was changed: from " ) + cc::j( coreItem[ Fields::value ] ) +
                      cc::trace( " to " ) + cc::j( itemValue[ Fields::value ] ) );
    }
    // Update scalable values in right way here: value : { value: number, scale: string }
    updateCachedItemFromBroadcast( searchKey, message );

    size_t changesEmitted = 0;
    // iterate all related expressions and re-calculate their values
    for( const auto& itemExpression : itemEntry.expressionNames() ) {
        Json answer;
        auto expressionIt = m_expressions.find( itemExpression );
        if( expressionIt == m_expressions.end() )
            continue;

        if( !expressionIt->second )
            continue;
        Expression& expression = *expressionIt->second;

        bool recalculate = false;
        if( isItemNameChanged ) {
            // check if item is addressed by name in this expression
            ItemsIndex itemsIndex;
            if( expression.indexItems( answer, itemsIndex ) ) {
                ItemsIndex::DeviceItemKey key = itemsIndex.getItemKeyByItemId( itemId );
                if( !key.deviceName().empty() && key.addressedByName() ) {
                    recalculate = true;
                }
            }
            else {
                LOGGER_ERROR( cc::error( "indexItems failed: answer: " ) + cc::j( answer ) );
                //  save an error and continue
                auto errorMessage = json_get_str( answer[ "error" ], "reason" );
                storeError( itemExpression, errorMessage );
                expression.error( errorMessage );
                expression.value( nullptr );
                continue;
            }
        }
        if( isItemValueChanged ) {
            recalculate = true;
        }

        if( recalculate ) {
            auto before = fetchValue( itemExpression );
            bool isOk = expression.process( answer, JSON_EMPTY_OBJ );
            auto after = fetchValue( itemExpression );

            bool hasValueChanged = before.hasValue && isOk && after.hasValue && before.value != after.value;
            bool hasBecomeAValue = ( !before.hasValue && !before.value.empty() ) && ( isOk && after.hasValue );
            if( hasValueChanged || hasBecomeAValue ) {
                emitExpressionValueChanged( itemExpression, after.value );
                changesEmitted++;
            }
        }
    }

    return changesEmitted > 0;
}

bool ExpressionManager::handleItemRemoved( const Json& message ) {
    using namespace Expressions::Resources;

    auto itemId = json_get_str( message, Fields::id );
    auto found = m_expressionsWithItem.find( itemId );
    if( found == m_expressionsWithItem.end() ) {
        return false;
    }

    {
        ItemEntry& itemEntry = found->second;

        // iterate all related expressions and set errors
        for( const auto& itemExpression : itemEntry.expressionNames() ) {
            auto expressionIt = m_expressions.find( itemExpression );
            if( expressionIt == m_expressions.end() )
                continue;
            if( !expressionIt->second )
                continue;

            Expression& expression = *expressionIt->second;
            auto errorMessage = "Item has been removed: " + itemEntry.name();
            storeError( itemExpression, errorMessage );
            expression.error( errorMessage );
        }
    }
    m_expressionsWithItem.erase( found );
    m_itemCache.removeItem( ValueSourceKey( m_ownControllerId, itemId ) );

    return true;
}

bool ExpressionManager::handleItemAdded( const Json& message ) {
    LOGGER_TRACE( cc::j( message ) );
    // update all expressions related to the device/item: set value
    size_t changesEmitted = 0;

    for( auto& it : m_expressions ) {
        auto& expressionPtr = it.second;
        if( !expressionPtr )
            continue;
        Expression& expression = *expressionPtr;
        if( expression.error().empty() )
            continue;

        // if there is some error, try to recheck
        Json answer = Json::object();
        bool isOk;
        std::tie( isOk, std::ignore ) = expression.checkParams( answer, expression.params(), nullptr );
        if( !isOk )
            continue;

        if( !expression.process( answer, JSON_EMPTY_OBJ ) )
            continue;

        auto result = fetchValue( expression.name() );
        if( !result.isOk() )
            continue;

        emitExpressionValueChanged( expression.name(), result.value );
        changesEmitted++;
    }
    return changesEmitted > 0;
}

bool ExpressionManager::handleDeviceUpdated( const Json& message ) {
    LOGGER_TRACE( cc::j( message ) );
    auto deviceId = json_get_str( message, Resources::Fields::id );
    auto deviceName = json_get_str( message, Resources::Fields::name );
    if( deviceName.empty() )
        return false;

    auto rawItems = m_core.getItemsByDeviceId( deviceId );
    size_t changesEmitted = 0;

    for( auto& it : m_expressions ) {
        auto& expressionPtr = it.second;
        if( !expressionPtr )
            continue;
        Expression& expression = *expressionPtr;
        bool hasError = !expression.error().empty();

        int hasItems = 0;
        for( const auto& itemIt : rawItems ) {
            auto itemId = json_get_str( itemIt, Resources::Fields::id );
            if( itemId.empty() )
                continue;

            if( expression.hasItemId( itemId ) ) {
                hasItems++;
            }
        }

        if( !hasItems )
            continue;

        Json answer = Json::object();
        bool isOk;
        std::tie( isOk, std::ignore ) = expression.checkParams( answer, expression.params(), nullptr );
        if( hasError && !isOk )
            continue;

        Json valueBefore;
        bool valueBeforeOk = false;
        if( !hasError ) {
            auto result = fetchValue( expression.name() );
            valueBeforeOk = result.isOk();
            valueBefore = std::move( result.value );
        }

        isOk = expression.process( answer, JSON_EMPTY_OBJ );

        Json valueAfter;
        if( isOk ) {
            auto result = fetchValue( expression.name() );
            isOk = result.isOk();
            valueAfter = std::move( result.value );
        }
        // hasError && isOk - emit anyway
        // !hasError && isOk && value changed - emit
        bool doEmit = ( isOk && hasError ) || ( isOk && !hasError && valueBeforeOk && valueBefore != valueAfter );
        if( doEmit ) {
            emitExpressionValueChanged( expression.name(), valueAfter );
            changesEmitted++;
        }
    }

    return changesEmitted > 0;
}

void ExpressionManager::emitExpressionValueChanged( const std::string& expressionName, const Json& value ) {
    LOGGER_TRACE( cc::trace( expressionName ) + ": " + cc::j( value ) );
    if( m_valueChangedCallback ) {
        m_valueChangedCallback( expressionName, value );
    }
}

void ExpressionManager::wrongParams( const std::string& fieldName, JsonObject& answer ) {
    wrongParam( fieldName, answer );
}

HubPlatform::CoreDRemoteInterface::CoreDInterface& ExpressionManager::getRemoteInterface() {
    return m_core;
}

void ExpressionManager::updateItemEntries( const std::string& name, const Expression& expression ) {
    std::set< ItemId > items = expression.getItemIds();
    for( const auto& itemId : items ) {
        auto it = m_expressionsWithItem.find( itemId );
        if( it == m_expressionsWithItem.end() ) {
            ItemEntry itemEntry;
            itemEntry.expressionNames().insert( name );
            JsonObject itemRaw = JSON_EMPTY_OBJ;
            itemEntry.name( json_get_str( itemRaw, Resources::Fields::name ) );
            m_expressionsWithItem.emplace( itemId, std::move( itemEntry ) );
        }
        else {
            ItemEntry& itemEntry = it->second;
            itemEntry.expressionNames().insert( name );
        }
    }
}

bool ExpressionManager::setVariableExpression( const std::string& name, TypedVariableExpressionUPtr expressionPtr ) {
    if( name.empty() || !expressionPtr )
        return false;
    LOGGER_TRACE( name );
    auto& newExpression = *expressionPtr;
    auto prevExpressionPtr = removeExpression( name );

    updateItemEntries( name, newExpression );
    insertExpression( name,
                      newExpression.code(),
                      newExpression.params(),
                      newExpression.valueType(),
                      newExpression.metadata(),
                      newExpression.value(),
                      newExpression.variable(),
                      newExpression.error() );

    if( !newExpression.value().empty() ) {
        this->storeValue( name, newExpression.value(), newExpression.valueError() );
        bool prevValueOk = prevExpressionPtr && !prevExpressionPtr->value().empty();
        if( prevValueOk && !newExpression.value().empty() && newExpression.value() != prevExpressionPtr->value() ) {
            emitExpressionValueChanged( newExpression.name(), newExpression.value() );
        }
    }
    else if( !newExpression.error().empty() ) {
        this->storeError( name, newExpression.error() );
    }

    auto result = m_valueExpressions.emplace( name, std::move( expressionPtr ) );
    return result.second;
}

bool ExpressionManager::setExpression( const std::string& name, TypedExpressionUPtr expressionPtr ) {
    if( name.empty() || !expressionPtr )
        return false;
    auto& newExpression = *expressionPtr;
    auto prevExpressionPtr = removeExpression( name );
    updateItemEntries( name, newExpression );

    insertExpression( name,
                      newExpression.code(),
                      newExpression.params(),
                      newExpression.valueType(),
                      newExpression.metadata(),
                      {},
                      newExpression.variable(),
                      newExpression.error() );

    if( !newExpression.value().empty() ) {
        this->storeValue( name, newExpression.value(), newExpression.valueError() );

        bool prevValueOk = prevExpressionPtr && !prevExpressionPtr->value().empty();
        if( prevValueOk && !newExpression.value().empty() && newExpression.value() != prevExpressionPtr->value() ) {
            emitExpressionValueChanged( newExpression.name(), newExpression.value() );
        }
    }
    else if( !newExpression.error().empty() ) {
        this->storeError( name, newExpression.error() );
    }

    auto result = m_expressions.emplace( name, std::move( expressionPtr ) );
    return result.second;
}

/** @brief Removes all references to an expression with name 'name' from ExpressionManager.
 *  @returns Unique pointer to a previous version of an expression.
 */
ExpressionUPtr ExpressionManager::removeExpression( const std::string& name ) {
    if( name.empty() )
        return nullptr;
    for( auto& itemEntryIt : m_expressionsWithItem ) {
        itemEntryIt.second.removeExpressionName( name );
    }
    deleteExpression( name );
    auto foundIt = m_expressions.find( name );
    if( foundIt != m_expressions.end() ) {
        auto tmpPtr = std::move( foundIt->second );
        m_expressions.erase( name );
        return std::move( tmpPtr );
    }

    auto it = m_valueExpressions.find( name );
    if( it != m_valueExpressions.end() ) {
        auto tmpPtr = std::move( it->second );
        m_valueExpressions.erase( name );
        return std::move( tmpPtr );
    }

    return nullptr;
}

/** @brief Need to cache item values gotten from hub.itm.updated broadcasts,
 * because coreInterface.getItem() always contain old version of value. Gets cached version first,
 * if not present, reads from a remote interface. The value should be maintained by checking item.updated broadcasts. */
JsonObject ExpressionManager::fetchCachedItem( const std::string& itemId, const std::string& controllerId ) {
    SceneEngine::ValueSourceKey searchKey = makeSearchKey( itemId );
    JsonObject coreItem = JSON_EMPTY_OBJ;
    if( m_itemCache.fetchItem( searchKey, coreItem ) ) {
        return coreItem;
    }
    return nullptr;
}

void ExpressionManager::onValueChanged( OnValueChangedCallback callback ) {
    m_valueChangedCallback = std::move( callback );
}

SceneEngine::ValueSourceKey ExpressionManager::makeSearchKey( const std::string& itemId ) {
    return { m_ownControllerId, itemId };
}

ItemCacheBase& ExpressionManager::itemCache() {
    return m_itemCache;
}

void ExpressionManager::ItemEntry::name( const std::string& value ) {
    m_name = value;
}

const std::string& ExpressionManager::ItemEntry::name() const {
    return m_name;
}

ExpressionManager::ItemEntry::ExpressionNames& ExpressionManager::ItemEntry::expressionNames() {
    return m_expressionNames;
}

bool ExpressionManager::ItemEntry::removeExpressionName( const std::string& name ) {
    return m_expressionNames.erase( name ) > 0;
}

}  // namespace Expressions
}  // namespace SceneEngine
