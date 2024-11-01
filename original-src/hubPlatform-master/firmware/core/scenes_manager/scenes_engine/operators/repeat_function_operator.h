#pragma once
#include "scenes_manager/scenes_engine/operators/function_operator.h"

#include <chrono>
#include <deque>

namespace SceneEngine {

class RepeatFunctionOperator : public FunctionOperator {
public:
    explicit RepeatFunctionOperator();
    ~RepeatFunctionOperator();

    void setOptions( int times, std::chrono::milliseconds interval );
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;

private:
    bool saveCurrentState( AbstractOperator::OperatorState newState );
    AbstractOperator::OperatorState changeCurrentState( AbstractOperator::OperatorState newState );
    AbstractOperator::OperatorState getCurrentState() const;

    using TimePoint = std::chrono::time_point< std::chrono::system_clock >;
    static TimePoint getNow();
    static std::chrono::milliseconds diffFromNow( TimePoint transition );

    unsigned countTrueTransitions() const;
    void moveTimeWindow();
    void notifyStates();
    bool checkExitCondition();
    void saveFalseState( OperatorState newChildrenState );

private:
    // Options
    int m_times{ 0 };  //!< Number of times the event should pass to true state before the function becomes True
    std::chrono::milliseconds m_intervalMilliseconds{ 0 };  //!< Period for which these transition should be completed.

    // Current state of a function, initial state is unknown
    AbstractOperator::OperatorState m_currentState{ OperatorState::False };

    struct Transition {
        Transition( AbstractOperator::OperatorState newState, TimePoint now );

    public:
        AbstractOperator::OperatorState m_currentState{ OperatorState::False };  // Current state of a transition
        TimePoint m_transition;
    };
    using Transitions = std::deque< Transition >;
    Transitions m_transitions;
};

}  // namespace SceneEngine