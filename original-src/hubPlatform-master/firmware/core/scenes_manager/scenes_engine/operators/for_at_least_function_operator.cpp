#include "scenes_manager/scenes_engine/operators/for_at_least_function_operator.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

ForAtLeastFunctionOperator::ForAtLeastFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop )
: m_taskLoop( taskLoop ) {}

ForAtLeastFunctionOperator::~ForAtLeastFunctionOperator() {
    stopTimer();
}

void ForAtLeastFunctionOperator::setOptions( std::chrono::milliseconds interval ) {
    m_intervalMilliseconds = interval;
    setInterceptSceneState( true );
}

AbstractOperator::OperatorState ForAtLeastFunctionOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !isEnabled() || getChildren().empty() ) {
        return Disabled;
    }

    // Check children state, use inherited AndOperator::checkChildren(), not own checkChildren()
    OperatorState changedChildrenState = AndOperator::checkChildren();

    // Evaluate if child state change affects the whole operator state change
    for( const auto& condition : getChildren() ) {
        if( condition == child ) {
            if( state == False ) {
                // modify state of a child
                AbstractOperator::checkParent( child, False );
                // check if state of all children is changed
                // FOR/LEAST: only take in account the state of children, not a current state. When all children are
                // true, ony after that the timer is started
                changedChildrenState = AndOperator::checkChildren();
            }
            break;
        }
    }

    if( changedChildrenState == False ) {
        auto prevState = getCurrentChildrenState();

        if( prevState != changedChildrenState ) {
            auto timeDiff = diffFromNow();
            saveCurrentChildrenState( False );

            LOGGER_TRACE( cc::trace( "for/least: False, stop timer" ) );
            stopTimer();
            saveCurrentOperatorState( False );
            notifyOperatorStateEvent( False );
            auto sceneState2 = AbstractOperator::checkParent( child, False );
            notifySceneStateEvent( sceneState2 );
        }
    }
    else if( changedChildrenState == True ) {
        bool isStateChanged = saveCurrentChildrenState( changedChildrenState );

        if( isStateChanged ) {
            stopTimer();
            saveCurrentOperatorState( False );

            notifyOperatorStateEvent( False );

            auto sceneState2 = AbstractOperator::checkParent( child, False );
            notifySceneStateEvent( sceneState2 );

            startTimer();
            return False;
        }
        else {
            if( m_timer != nullptr ) {
                // timer is present, return True to activate a scene
                LOGGER_DEBUG( cc::debug( "for/least: OK, sceneState = " + cc::flag( true ) ) );
                return True;
            }
        }
    }
    return False;
}

/** Returns a state of children and the current operator state. True only if both underlying AND() operator is true and
 * the timer event has occurred. */
AbstractOperator::OperatorState ForAtLeastFunctionOperator::checkChildren() {
    OperatorState andState = AndOperator::checkChildren();
    OperatorState thisOperatorState = getOperatorState();

    auto state = toOperatorState( toBool( andState ) && toBool( thisOperatorState ) );
    return state;
}

void ForAtLeastFunctionOperator::saveTransitionMark() {
    m_transition = std::chrono::system_clock::now();
}

std::chrono::milliseconds ForAtLeastFunctionOperator::diffFromNow() const {
    auto now = std::chrono::system_clock::now();
    auto millisecondsPassed = std::chrono::duration_cast< std::chrono::milliseconds >( now - m_transition );
    return millisecondsPassed;
}

bool ForAtLeastFunctionOperator::saveCurrentChildrenState( OperatorState newState ) {
    auto prevState = m_currentChildState;
    m_currentChildState = newState;
    if( prevState != m_currentChildState ) {
        auto diff = diffFromNow();
        LOGGER_TRACE_RAW( cc::trace( "for/least: transition occurred: " ) + cc::flag( toBool( newState ) ) +
                          cc::trace( ", delta from previous: " ) + cc::num10( diff.count() ) + cc::trace( " ms" ) );
        saveTransitionMark();

        return true;
    }
    return false;
}

AbstractOperator::OperatorState ForAtLeastFunctionOperator::getCurrentChildrenState() const {
    return m_currentChildState;
}

void ForAtLeastFunctionOperator::stopTimer() {
    if( m_timer ) {
        m_timer->stop();
        m_timer.reset();
    }
}

void ForAtLeastFunctionOperator::startTimer() {
    using namespace HubPlatform::Services;
    auto forOperator = std::static_pointer_cast< ForAtLeastFunctionOperator >( shared_from_this() );
    saveCurrentOperatorState( False );

    m_timer = m_taskLoop.createPostponedTask( [forOperator]( PostponedTask& ) {
        forOperator->saveCurrentOperatorState( True );

        auto sceneState = forOperator->AbstractOperator::checkParent( forOperator, True );
        forOperator->notifyOperatorStateEvent( True );
        LOGGER_DEBUG_RAW( cc::debug( "for/least timer: notifySceneStateEvent called, sceneState = " ) +
                          cc::flag( toBool( sceneState ) ) );
        forOperator->notifySceneStateEvent( sceneState );
        forOperator->stopTimer();
    } );

    LOGGER_TRACE_RAW( cc::trace( "for/least: start new timer" ) );
    m_timer->start( TaskLoop::Interval( m_intervalMilliseconds ), false );
}

void ForAtLeastFunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

void ForAtLeastFunctionOperator::saveCurrentOperatorState( OperatorState newState ) {
    m_forState = newState;
}

AbstractOperator::OperatorState ForAtLeastFunctionOperator::getOperatorState() const {
    return m_forState;
}

}  // namespace SceneEngine
