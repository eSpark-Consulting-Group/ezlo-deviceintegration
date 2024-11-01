#include "scenes_manager/scenes_engine/expressions/expression_value_provider.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"
#include "json_utils/json_utils.h"
#include "firmware/core/scenes_manager/modify_scene/resources.h"
#include "resources.h"

namespace SceneEngine {
namespace Expressions {

SceneEngine::ItemValueProviderPtr makeExpressionValueProvider( ExpressionManager& manager,
                                                               const std::string& expressionName ) {
    return std::make_shared< ExpressionValueProvider >( manager, expressionName );
}

ExpressionValueProvider::ExpressionValueProvider( ExpressionManager& manager, std::string expressionName )
: m_manager( manager )
, m_expressionName( std::move( expressionName ) ) {}

bool ExpressionValueProvider::fetchItemValue() {
    auto result = m_manager.fetchValue( m_expressionName );

    if( result.isOk() ) {
        auto valueType = m_manager.findValueType( m_expressionName );
        if( !valueType.empty() )
            m_cachedValue = ItemValueWrapper::fromValueAndType( result.value, valueType );
        else
            m_cachedValue = ItemValueWrapper( std::move( result.value ) );

        m_cachedError = {};
        return true;
    }

    m_cachedValue = ItemValueWrapper();
    m_cachedError = std::move( result.error );
    return false;
}

const SceneEngine::ItemValueWrapper& ExpressionValueProvider::getCachedItemValue() const {
    return m_cachedValue;
}

const Json& ExpressionValueProvider::getCachedError() const {
    return m_cachedError;
}

bool ExpressionValueProvider::hasValueKey() const {
    return false;
}

const ValueSourceKey& ExpressionValueProvider::getValueKey() const {
    throw EventException( "Unexpected call of \"ItemValueProvider::getValueKey\"" );
}

}  // namespace Expressions
}  // namespace SceneEngine
