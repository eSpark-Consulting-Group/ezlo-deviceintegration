#pragma once
#include "scenes_manager/scenes_engine/operators/function_operator.h"
#include "service_interfaces/task_loop.h"

namespace SceneEngine {

class ForLessFunctionOperator : public FunctionOperator {
public:
    ForLessFunctionOperator();
    ~ForLessFunctionOperator() override;

    void setOptions( std::chrono::milliseconds interval );
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;

protected:
    void saveTransitionMark();
    std::chrono::milliseconds diffFromNow() const;
    bool saveCurrentState( OperatorState newState );
    OperatorState getCurrentState() const;

private:
    std::chrono::milliseconds m_intervalMilliseconds{ 0 };
    OperatorState m_currentState{ OperatorState::False };
    std::chrono::time_point< std::chrono::system_clock > m_transition;
};

}  // namespace SceneEngine