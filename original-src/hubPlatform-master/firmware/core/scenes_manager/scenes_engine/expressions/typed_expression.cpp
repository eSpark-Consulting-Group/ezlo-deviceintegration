#include "scenes_manager/scenes_engine/expressions/typed_expression.h"

#include <utility>
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "cored/remote_interface/cored_interface.h"

namespace SceneEngine {
namespace Expressions {

TypedExpression::TypedExpression( ExpressionManager& manager,
                                  std::string expressionName,
                                  std::string code,
                                  Json params,
                                  std::string valueType,
                                  Json metadata,
                                  bool variable )
: Expression( manager, std::move( expressionName ), std::move( code ), std::move( params ) )
, m_valueType( std::move( valueType ) )
, m_metadata( std::move( metadata ) )
, m_variable( variable ) {}

const std::string& TypedExpression::valueType() const {
    return m_valueType;
}

const Json& TypedExpression::metadata() const {
    return m_metadata;
}

bool TypedExpression::variable() const {
    return m_variable;
}

}  // namespace Expressions
}  // namespace SceneEngine
