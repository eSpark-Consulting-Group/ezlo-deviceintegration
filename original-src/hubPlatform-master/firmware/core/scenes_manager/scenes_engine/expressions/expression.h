#pragma once
#include "json_utils/json_fwd.h"
#include <set>
#include <memory>

namespace SceneEngine {
namespace Expressions {

class ExpressionManager;
class ItemsIndex;

class Expression {
public:
    using ItemId = std::string;
    using FetchItemValue =
        std::function< void( const std::string& deviceName, const std::string& itemName, const Json& item ) >;

public:
    Expression( ExpressionManager& manager, std::string expressionName, std::string code, Json params );
    const std::string& name() const;
    const std::string& code() const;
    const Json& params() const;

    const Json& value() const;
    void value( Json expressionValue );
    const Json& error() const;
    void error( Json expressionError );

    const Json& valueError() const;
    void setValueError( Json expressionError );

    void addItemId( ItemId itemId );
    void deleteItemId( const ItemId& itemId );
    bool hasItemId( const ItemId& itemId ) const;
    void clearItemIds();
    std::set< ItemId > getItemIds() const;

    virtual bool process( Json& answer, Json valueError );
    virtual bool process( Json& answer, ItemsIndex& itemsIndex, Json& valueError );
    bool indexItems( Json& answer, ItemsIndex& itemsIndex );
    bool storeValue( std::string& name, Json& value );
    bool storeError( std::string& name, Json& error );
    bool storeValueAndError( std::string& name, Json& value, Json& error );
    std::pair< bool, std::string > checkParams( Json& answer,
                                                const Json& params,
                                                const FetchItemValue& fetchItemCallback );

private:
    std::pair< bool, std::string > checkParamsItems( Json& answer,
                                                     const JsonArray& items,
                                                     const FetchItemValue& fetchItemCallback );
    std::pair< bool, std::string > checkParamsDeviceItemsNames( Json& answer,
                                                                const JsonArray& deviceItemNames,
                                                                const FetchItemValue& fetchItemCallback );

private:
    ExpressionManager& m_manager;
    std::string m_code;
    Json m_params;

    Json m_value;
    std::set< ItemId > m_items;

protected:
    const std::string m_name;
    Json m_error;
    Json m_valueError;
};

using ExpressionUPtr = std::unique_ptr< Expression >;

}  // namespace Expressions
}  // namespace SceneEngine