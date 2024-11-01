#include "scenes_manager/scenes_engine/operators/pulse_function_operator.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <limits>

namespace SceneEngine {

PulseFunctionOperator::PulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                              std::chrono::seconds truePeriod,
                                              std::chrono::seconds falsePeriod )
: PulseFunctionOperator( taskLoop, truePeriod, falsePeriod, std::numeric_limits< int >::max() ) {}

PulseFunctionOperator::PulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                              std::chrono::seconds truePeriod,
                                              std::chrono::seconds falsePeriod,
                                              int times )
: m_taskLoop( taskLoop )
, m_truePeriod( truePeriod )
, m_falsePeriod( falsePeriod )
, m_activateTimes( times ) {
    validateParams( truePeriod, falsePeriod, times );
    setInterceptSceneState( true );
}

PulseFunctionOperator::~PulseFunctionOperator() {
    if( m_pulseGenerationTask )
        m_pulseGenerationTask->stop();
}

AbstractOperator::OperatorState PulseFunctionOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }

    const OperatorState childrenState = checkChildren();
    bool sendStateEvents = false;

    if( !m_pulseGenerationTask ) {
        bool launchPulseGenerator = m_actualState == OperatorState::False && childrenState == OperatorState::True;
        if( launchPulseGenerator ) {
            startPulseGeneration();
            sendStateEvents = true;
        }
    }

    m_actualState = childrenState;

    OperatorState shownState = m_actualState;
    if( m_pulseGenerationTask ) {
        shownState = toOperatorState( m_pulseState );
        m_shouldStop = m_actualState != OperatorState::True;
        if( m_shouldStop && m_pulseState == false ) {
            m_pulseGenerationTask->stop();
            m_pulseGenerationTask.reset();
        }
    }
    else if( m_actualState == OperatorState::True ) {
        // pulse generator created requested amount of pulses and exited, now keep last value
        shownState = toOperatorState( m_pulseState );
    }

    const OperatorState subtreeState = AbstractOperator::checkParent( shared_from_this(), shownState );
    if( sendStateEvents ) {
        notifyOperatorStateEvent( shownState );
        notifySceneStateEvent( subtreeState );
    }
    return subtreeState;
}

AbstractOperator::OperatorState PulseFunctionOperator::checkChildren() {
    return AndOperator::checkChildren();
}

void PulseFunctionOperator::startPulseGeneration() {
    auto sharedThis = std::static_pointer_cast< PulseFunctionOperator >( shared_from_this() );
    auto pulseGenerationFun = [sharedThis]( HubPlatform::Services::PostponedTask& task ) {
        const bool newPulseState = !sharedThis->m_pulseState;
        if( newPulseState == true && sharedThis->m_timesWasActivated >= sharedThis->m_activateTimes ) {
            sharedThis->m_pulseGenerationTask.reset();
            return;
        }

        sharedThis->m_pulseState = newPulseState;

        const OperatorState operatorState = toOperatorState( newPulseState );
        sharedThis->notifyOperatorStateEvent( operatorState );
        const OperatorState subtreeState = sharedThis->AbstractOperator::checkParent( sharedThis, operatorState );
        sharedThis->notifySceneStateEvent( subtreeState );

        if( sharedThis->m_pulseState == false && sharedThis->m_shouldStop ) {
            sharedThis->m_pulseGenerationTask.reset();
            return;
        }

        if( newPulseState ) {
            sharedThis->m_timesWasActivated++;
            task.start( sharedThis->m_truePeriod, false );
        }
        else {
            task.start( sharedThis->m_falsePeriod, false );
        }
    };

    m_pulseGenerationTask = m_taskLoop.createPostponedTask( pulseGenerationFun );
    m_pulseGenerationTask->start( m_truePeriod, false );

    m_shouldStop = false;
    m_pulseState = true;
    m_timesWasActivated = 1;
}

void PulseFunctionOperator::validateParams( std::chrono::seconds truePeriod,
                                            std::chrono::seconds falsePeriod,
                                            int times ) {
    using namespace Model::Resources::FunctionFields;

    if( truePeriod <= std::chrono::seconds::zero() ) {
        std::string message = std::string( Pulse ) + ": '" + TruePeriod + "' parameter should be a positive integer";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message );
    }
    if( falsePeriod < std::chrono::seconds::zero() ) {
        std::string message =
            std::string( Pulse ) + ": '" + FalsePeriod + "' parameter should be a non-negative integer";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message );
    }
    if( times <= 0 ) {
        std::string message = std::string( Pulse ) + ": '" + Times + "' parameter should be a positive integer";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message );
    }
}

void PulseFunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine
