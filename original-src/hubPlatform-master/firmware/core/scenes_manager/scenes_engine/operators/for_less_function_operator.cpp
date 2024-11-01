#include "scenes_manager/scenes_engine/operators/for_less_function_operator.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

ForLessFunctionOperator::ForLessFunctionOperator() = default;

ForLessFunctionOperator::~ForLessFunctionOperator() = default;

void ForLessFunctionOperator::setOptions( std::chrono::milliseconds interval ) {
    m_intervalMilliseconds = interval;
    setInterceptSceneState( true );
}

AbstractOperator::OperatorState ForLessFunctionOperator::checkParent( SharedOperator child, OperatorState state ) {
    // check enabled and empty
    if( !isEnabled() || getChildren().empty() ) {
        return Disabled;
    }

    auto childrenState = checkChildren();
    OperatorState changedChildrenState = childrenState;

    // Evaluate if child state change affects the whole operator state change
    for( const auto& condition : getChildren() ) {
        if( condition == child ) {
            if( state == False ) {
                // modify state of a child
                AbstractOperator::checkParent( child, False );
                // check if state of all children is changed
                changedChildrenState = checkChildren();
            }
            break;
        }
    }

    // False case
    if( changedChildrenState == False ) {
        // state transition to false
        auto prevState = getCurrentState();

        if( prevState != changedChildrenState ) {
            auto timeDiff = diffFromNow();
            saveCurrentState( False );

            if( std::chrono::duration_cast< std::chrono::milliseconds >( timeDiff ) < m_intervalMilliseconds ) {
                // Change state to True, and immediately to False after that
                notifyOperatorStateEvent( True );
                auto sceneState = AbstractOperator::checkParent( child, True );
                LOGGER_DEBUG( cc::debug( "for/less: OK, sceneState = " + cc::flag( toBool( sceneState ) ) ) );
                notifySceneStateEvent( sceneState );

                notifyOperatorStateEvent( False );
                auto sceneState2 = AbstractOperator::checkParent( child, False );
                notifySceneStateEvent( sceneState2 );

                return False;
            }
            else {
                LOGGER_TRACE( cc::trace( "for/less: Not OK" ) );
            }
        }
    }

    // True case
    // Do not take in account the value of child state, but only the value of all children
    if( toBool( changedChildrenState ) ) {
        saveCurrentState( changedChildrenState );
        return False;
    }
    return False;
}

AbstractOperator::OperatorState ForLessFunctionOperator::checkChildren() {
    return AndOperator::checkChildren();
}

void ForLessFunctionOperator::saveTransitionMark() {
    m_transition = std::chrono::system_clock::now();
}

std::chrono::milliseconds ForLessFunctionOperator::diffFromNow() const {
    auto now = std::chrono::system_clock::now();
    auto millisecondsPassed = std::chrono::duration_cast< std::chrono::milliseconds >( now - m_transition );
    return millisecondsPassed;
}

bool ForLessFunctionOperator::saveCurrentState( AbstractOperator::OperatorState newState ) {
    auto prevState = m_currentState;
    m_currentState = newState;
    if( prevState != m_currentState ) {
        auto diff = diffFromNow();
        LOGGER_TRACE( cc::trace( "transition occurred: " ) + cc::flag( toBool( newState ) ) +
                      cc::trace( ", delta from previous: " ) + cc::num10( diff.count() ) + cc::trace( " ms" ) );
        saveTransitionMark();

        return true;
    }
    return false;
}

AbstractOperator::OperatorState ForLessFunctionOperator::getCurrentState() const {
    return m_currentState;
}

void ForLessFunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine
