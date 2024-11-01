#pragma once
#include "scenes_manager/scenes_engine/operators/function_operator.h"

namespace SceneEngine {

class LatchFunctionOperator : public FunctionOperator {
public:
    explicit LatchFunctionOperator( bool isLatchEnables = false );

    void registerChild( AbstractOperator::SharedOperator child ) override;
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;

    void closeLatch();
    void openLatch();
    bool isLatched() const;

private:
    OperatorState saveState( AbstractOperator::OperatorState newState );

private:
    bool m_isLatched{ false };
    AbstractOperator::OperatorState m_currentState{ OperatorState::False };
};

}  // namespace SceneEngine
