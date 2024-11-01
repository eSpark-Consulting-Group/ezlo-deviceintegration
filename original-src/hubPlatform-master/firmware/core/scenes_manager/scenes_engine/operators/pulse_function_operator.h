#pragma once

#include "scenes_manager/scenes_engine/operators/function_operator.h"
#include "service_interfaces/task_loop.h"

namespace SceneEngine {

class PulseFunctionOperator : public FunctionOperator {
public:
    PulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                           std::chrono::seconds truePeriod,
                           std::chrono::seconds falsePeriod );
    PulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                           std::chrono::seconds truePeriod,
                           std::chrono::seconds falsePeriod,
                           int times );
    ~PulseFunctionOperator();

    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;

private:
    void startPulseGeneration();

    static void validateParams( std::chrono::seconds truePeriod, std::chrono::seconds falsePeriod, int times );

private:
    HubPlatform::Services::TaskLoop& m_taskLoop;
    HubPlatform::Services::PostponedTaskPtr m_pulseGenerationTask;

    std::chrono::seconds m_truePeriod{ 0 };
    std::chrono::seconds m_falsePeriod{ 0 };
    int m_activateTimes{ 0 };

    OperatorState m_actualState{ OperatorState::False };
    int m_timesWasActivated{ 0 };
    bool m_pulseState{ false };
    bool m_shouldStop{ false };
};

}  // namespace SceneEngine
