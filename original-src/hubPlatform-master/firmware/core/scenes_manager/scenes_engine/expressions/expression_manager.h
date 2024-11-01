#pragma once
#include "scenes_manager/scenes_engine/expressions/items_index.h"
#include "scenes_manager/scenes_engine/expressions/typed_expression.h"
#include "scenes_manager/scenes_engine/expressions/typed_variable_expression.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache_base.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_wrapper.h"
#include <functional>

class IDbClient;
namespace HubPlatform {
namespace CoreDRemoteInterface {
class CoreDInterface;
}
}  // namespace HubPlatform

namespace SceneEngine {
namespace Expressions {

class ExpressionManager {
public:
    using OnValueChangedCallback = std::function< void( std::string expressionName, Json updatedValue ) >;

public:
    virtual ~ExpressionManager() = default;
    ExpressionManager( HubPlatform::CoreDRemoteInterface::CoreDInterface& coreRemote,
                       IDbClient& dbClient,
                       SceneEngine::ItemCacheBase& itemCache,
                       std::string controllerId );
    void onValueChanged( OnValueChangedCallback callback );

    void setUp();
    void tearDown();

    Json findByName( const std::string& name ) const;
    bool insertExpression( const std::string& name,
                           const std::string& code,
                           const Json& params,
                           const std::string& typeValue,
                           const Json& metadata,
                           const Json& value,
                           bool variable,
                           const Json& error );
    bool deleteExpression( const std::string& name );
    bool hasExpression( const std::string& name ) const;
    std::vector< Json > findExpressionsBy( const Json& ) const;
    static std::pair< bool, Json > calculateExpression( Json& answer,
                                                        const std::string& code,
                                                        const ItemsIndex& itemsIndex );
    bool storeValue( const std::string& name, const Json& value, const Json& error );
    bool storeError( const std::string& name, const Json& value );

    struct FetchResult {
        enum class Status { Ok, NotFound, Error, ValueError, ValueNotFound };
        const Status status = Status::NotFound;
        Json error;
        Json value;
        const bool hasValue = false;

        bool isOk() const {
            return status == Status::Ok;
        }

    protected:
        friend ExpressionManager;
        FetchResult( Status status, Json error );
        FetchResult( Status status, Json error, Json value );
    };
    FetchResult fetchValue( const std::string& name ) const;
    std::string findValueType( const std::string& name );

    bool handleItemUpdated( const Json& message );
    bool handleItemRemoved( const Json& message );
    bool handleItemAdded( const Json& message );
    bool handleDeviceUpdated( const Json& message );

    bool setExpression( const std::string& name, TypedExpressionUPtr expressionPtr );
    bool setVariableExpression( const std::string& name, TypedVariableExpressionUPtr expressionPtr );
    ExpressionUPtr removeExpression( const std::string& name );
    HubPlatform::CoreDRemoteInterface::CoreDInterface& getRemoteInterface();
    static void wrongParams( const std::string& fieldName, JsonObject& answer );
    JsonObject fetchCachedItem( const std::string& itemId, const std::string& controllerId );
    ItemCacheBase& itemCache();
    static bool extractValueFromBroadcast( const Json& message, Json& convertedValue );

private:
    bool storeField( const std::string& name, const std::string& field, const Json& value, const Json& error );
    void emitExpressionValueChanged( const std::string& expressionName, const Json& valueOrError );
    SceneEngine::ValueSourceKey makeSearchKey( const std::string& itemId );
    void updateItemEntries( const std::string& name, const Expression& expression );
    void updateCachedItemFromBroadcast( const ValueSourceKey& searchKey, const Json& message );

private:
    std::string m_ownControllerId;
    // TODO: move to transport abstraction (could belong to another controller): use ItemCache here
    HubPlatform::CoreDRemoteInterface::CoreDInterface& m_core;
    IDbClient& m_dbClient;

    using ExpressionName = std::string;
    using ItemId = std::string;
    class ItemEntry {
    public:
        using ExpressionNames = std::set< ExpressionName >;
        void name( const std::string& value );
        const std::string& name() const;
        ExpressionNames& expressionNames();
        bool removeExpressionName( const std::string& name );

    private:
        ExpressionNames m_expressionNames;
        std::string m_name;
    };
    std::map< ExpressionName, TypedExpressionUPtr > m_expressions;
    std::map< ExpressionName, TypedVariableExpressionUPtr > m_valueExpressions;
    std::map< ItemId, ItemEntry > m_expressionsWithItem;

    using DeviceId = std::string;
    using DeviceName = std::string;
    std::map< DeviceId, DeviceName > m_deviceNames;

    OnValueChangedCallback m_valueChangedCallback{ nullptr };
    SceneEngine::ItemCacheBase& m_itemCache;
};

}  // namespace Expressions
}  // namespace SceneEngine
