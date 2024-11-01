#include "scenes_manager/scenes_engine/expressions/typed_variable_expression.h"

#include <utility>
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "cored/remote_interface/cored_interface.h"

namespace SceneEngine {
namespace Expressions {

TypedVariableExpression::TypedVariableExpression( ExpressionManager& manager,
                                                  std::string expressionName,
                                                  Json value,
                                                  Json params,
                                                  std::string valueType,
                                                  Json metadata,
                                                  bool variable,
                                                  Json error )
: Expression( manager, std::move( expressionName ), "", std::move( params ) )
, m_value( std::move( value ) )
, m_valueType( std::move( valueType ) )
, m_metadata( std::move( metadata ) )
, m_variable( variable )
, m_error( std::move( error ) ) {}

const std::string& TypedVariableExpression::valueType() const {
    return m_valueType;
}

const Json& TypedVariableExpression::metadata() const {
    return m_metadata;
}

bool TypedVariableExpression::variable() const {
    return m_variable;
}

const Json& TypedVariableExpression::error() const {
    return m_error;
}

}  // namespace Expressions
}  // namespace SceneEngine
