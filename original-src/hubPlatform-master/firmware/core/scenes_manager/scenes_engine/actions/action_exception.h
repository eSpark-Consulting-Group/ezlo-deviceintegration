#pragma once

#include "scenes_manager/scenes_engine/actions/action_exception_data.h"

namespace SceneEngine {

namespace Expressions {
class ExpressionManager;
}  // namespace Expressions

class StatesRegistry;
class ItemValueWrapper;

class ActionExceptionError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ActionException {
public:
    enum class State { Satisfied, Dissatisfied };

    ActionException( ActionExceptionData exceptionData,
                     const StatesRegistry& statesRegistry,
                     const Expressions::ExpressionManager& expressions );
    State evaluate();

    static ItemValueWrapper getExpectedExpressionValue();

private:
    using BlockReference = ActionExceptionData::BlockReference;
    using ExpressionReference = ActionExceptionData::ExpressionReference;

    State evaluate( const BlockReference& blockReference ) const;
    State evaluate( const ExpressionReference& expressionReference ) const;

private:
    ActionExceptionData m_exceptionData;
    const StatesRegistry& m_statesRegistry;
    const Expressions::ExpressionManager& m_expressions;
};

}  // namespace SceneEngine
