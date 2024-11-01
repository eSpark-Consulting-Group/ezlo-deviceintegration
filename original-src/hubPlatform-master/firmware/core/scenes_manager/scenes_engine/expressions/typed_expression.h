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

class TypedExpression : public Expression {
public:
    using ItemId = std::string;
    using FetchItemValue =
        std::function< void( const std::string& deviceName, const std::string& itemName, const Json& item ) >;

public:
    TypedExpression( ExpressionManager& manager,
                     std::string expressionName,
                     std::string code,
                     Json params,
                     std::string typeName,
                     Json metadata,
                     bool variable );

    const std::string& valueType() const;
    const Json& metadata() const;
    bool variable() const;

private:
    std::string m_valueType;
    Json m_metadata;
    bool m_variable;
};

using TypedExpressionUPtr = std::unique_ptr< TypedExpression >;

}  // namespace Expressions
}  // namespace SceneEngine