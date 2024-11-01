#pragma once
#include "scenes_manager/scenes_engine/operators/function_operator.h"
#include "service_interfaces/task_loop.h"

namespace SceneEngine {

class ForAtLeastFunctionOperator : public FunctionOperator {
public:
    explicit ForAtLeastFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop );
    ~ForAtLeastFunctionOperator() override;

    void setOptions( std::chrono::milliseconds interval );
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;

public:
    void stopTimer();
    void startTimer();

protected:
    void saveTransitionMark();
    std::chrono::milliseconds diffFromNow() const;
    bool saveCurrentChildrenState( OperatorState newState );
    OperatorState getCurrentChildrenState() const;
    void saveCurrentOperatorState( OperatorState newState );
    OperatorState getOperatorState() const;

private:
    std::chrono::milliseconds m_intervalMilliseconds{ 0 };
    OperatorState m_currentChildState{ False };  //!< True when all underlying operators become True
    std::chrono::time_point< std::chrono::system_clock > m_transition;
    OperatorState m_forState{ False };  //!< True when FOR/AT_LEAST becomes true

    HubPlatform::Services::TaskLoop& m_taskLoop;
    HubPlatform::Services::PostponedTaskPtr m_timer;
};

}  // namespace SceneEngine