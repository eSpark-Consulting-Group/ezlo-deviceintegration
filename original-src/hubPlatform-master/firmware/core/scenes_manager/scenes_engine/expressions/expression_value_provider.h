#pragma once

#include "scenes_manager/scenes_engine/events/item_value_provider.h"

namespace SceneEngine {

class ValueSourceKey;

namespace Expressions {

class ExpressionManager;

class ExpressionValueProvider : public SceneEngine::ItemValueProviderInterface {
public:
    ExpressionValueProvider( ExpressionManager& manager, std::string expressionName );

public:
    bool fetchItemValue() override;
    const SceneEngine::ItemValueWrapper& getCachedItemValue() const override;
    const Json& getCachedError() const override;
    bool hasValueKey() const override;
    const ValueSourceKey& getValueKey() const override;

private:
    ExpressionManager& m_manager;
    std::string m_expressionName;
    SceneEngine::ItemValueWrapper m_cachedValue;
    Json m_cachedError;
};

SceneEngine::ItemValueProviderPtr makeExpressionValueProvider( ExpressionManager& manager,
                                                               const std::string& expressionName );

}  // namespace Expressions
}  // namespace SceneEngine
