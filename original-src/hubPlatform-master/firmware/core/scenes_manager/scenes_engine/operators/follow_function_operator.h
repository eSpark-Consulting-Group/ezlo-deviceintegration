#pragma once

#include "scenes_manager/scenes_engine/operators/function_operator.h"
#include "service_interfaces/task_loop.h"

namespace SceneEngine {

class FollowFunctionOperator : public FunctionOperator {
public:
    FollowFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop, std::chrono::seconds resetDelay );
    ~FollowFunctionOperator();

    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;

private:
    void processActualStateChanged( OperatorState newState );
    void scheduleReset();

    HubPlatform::Services::TaskLoop& m_taskLoop;
    HubPlatform::Services::PostponedTaskPtr m_delayedResetTask;

    std::chrono::seconds m_resetDelay;

    OperatorState m_actualState;
};

}  // namespace SceneEngine
