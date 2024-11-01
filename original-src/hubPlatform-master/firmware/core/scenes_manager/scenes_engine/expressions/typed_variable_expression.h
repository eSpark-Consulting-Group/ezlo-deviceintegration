#pragma once
#include "scenes_manager/scenes_engine/expressions/expression.h"
#include "json_utils/json_fwd.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include <set>
#include <memory>

namespace SceneEngine {
namespace Expressions {

class ExpressionManager;
class ItemsIndex;

class TypedVariableExpression : public Expression {
public:
    using ItemId = std::string;
    using FetchItemValue =
        std::function< void( const std::string& deviceName, const std::string& itemName, const Json& item ) >;

public:
    TypedVariableExpression( ExpressionManager& manager,
                             std::string expressionName,
                             Json value,
                             Json params,
                             std::string typeName,
                             Json metadata,
                             bool variable,
                             Json error );

    const std::string& valueType() const;
    const Json& metadata() const;
    bool variable() const;
    const Json& error() const;

private:
    Json m_value;
    std::string m_valueType;
    Json m_metadata;
    bool m_variable;
    Json m_error;
};

using TypedVariableExpressionUPtr = std::unique_ptr< TypedVariableExpression >;

}  // namespace Expressions
}  // namespace SceneEngine