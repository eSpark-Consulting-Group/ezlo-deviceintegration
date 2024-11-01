#include "scenes_manager/scenes_engine/operators/repeat_function_operator.h"

#include "scenes_manager/scenes_engine/exceptions.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

RepeatFunctionOperator::Transition::Transition( AbstractOperator::OperatorState newState,
                                                RepeatFunctionOperator::TimePoint now )
: m_currentState( newState )
, m_transition( now ) {}

RepeatFunctionOperator::RepeatFunctionOperator() = default;

RepeatFunctionOperator::~RepeatFunctionOperator() = default;

void RepeatFunctionOperator::setOptions( int times, std::chrono::milliseconds interval ) {
    m_times = times;
    m_intervalMilliseconds = interval;

    if( m_times < 2 ) {
        throw SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            std::string{ "Invalid number of times for REPEAT function. at least 2 expected, but have: " }
                .append( std::to_string( m_times ) )
                .c_str() );
    }
    if( interval.count() == 0 ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid interval for REPEAT function: cannot be zero" );
    }

    setInterceptSceneState( true );
}

AbstractOperator::OperatorState RepeatFunctionOperator::checkParent( AbstractOperator::SharedOperator child,
                                                                     AbstractOperator::OperatorState state ) {
    if( !isEnabled() || getChildren().empty() ) {
        return OperatorState::Disabled;
    }

    auto newChildrenState = checkChildren();

    if( getCurrentState() != newChildrenState ) {
        if( toBool( newChildrenState ) ) {
            saveCurrentState( newChildrenState );
            if( checkExitCondition() )
                return OperatorState::True;
        }
        else {
            saveFalseState( newChildrenState );
        }
    }
    else if( toBool( newChildrenState ) ) {
        return OperatorState::True;
    }
    return OperatorState::False;
}

void RepeatFunctionOperator::saveFalseState(
    AbstractOperator::OperatorState newChildrenState ) {  // first saved event should always be True
    if( !m_transitions.empty() ) {
        saveCurrentState( newChildrenState );
    }
    else {
        changeCurrentState( newChildrenState );
    }
}

bool RepeatFunctionOperator::checkExitCondition() {
    unsigned int trueTransitions = countTrueTransitions();
    if( trueTransitions < m_times )
        return false;
    while( !m_transitions.empty() && trueTransitions >= m_times ) {
        auto transition = m_transitions.front().m_transition;
        if( diffFromNow( transition ) <= m_intervalMilliseconds ) {
            LOGGER_TRACE_RAW( cc::trace( "function/repeat: time condition: OK, diff, ms: " ) +
                              cc::num10( diffFromNow( transition ).count() ) );

            notifyStates();
            m_transitions.clear();
            return true;
        }
        LOGGER_TRACE_RAW( cc::trace( "function/repeat: time condition: Failed, diff, ms: " ) +
                          cc::num10( diffFromNow( transition ).count() ) );
        // move window and repeat
        moveTimeWindow();
        trueTransitions = countTrueTransitions();
    }
    return false;
}

AbstractOperator::OperatorState RepeatFunctionOperator::checkChildren() {
    return AndOperator::checkChildren();
}

bool RepeatFunctionOperator::saveCurrentState( AbstractOperator::OperatorState newState ) {
    auto prevState = changeCurrentState( newState );
    if( prevState != m_currentState ) {
        m_transitions.emplace_back( newState, getNow() );

        return true;
    }
    return false;
}

AbstractOperator::OperatorState RepeatFunctionOperator::changeCurrentState( AbstractOperator::OperatorState newState ) {
    auto prevState = m_currentState;
    m_currentState = newState;
    return prevState;
}

AbstractOperator::OperatorState RepeatFunctionOperator::getCurrentState() const {
    return m_currentState;
}

RepeatFunctionOperator::TimePoint RepeatFunctionOperator::getNow() {
    return std::chrono::system_clock::now();
}

unsigned RepeatFunctionOperator::countTrueTransitions() const {
    return std::count_if( begin( m_transitions ), end( m_transitions ), []( const Transition& transition ) {
        return toBool( transition.m_currentState );
    } );
}

std::chrono::milliseconds RepeatFunctionOperator::diffFromNow( RepeatFunctionOperator::TimePoint transition ) {
    return std::chrono::duration_cast< std::chrono::milliseconds >( getNow() - transition );
}

void RepeatFunctionOperator::notifyStates() {
    auto thisOperator = shared_from_this();
    auto sceneState = FunctionOperator::checkParent( thisOperator, True );

    notifyOperatorStateEvent( True );
    LOGGER_DEBUG_RAW( cc::debug( "function/repeat: notifySceneStateEvent called, sceneState = " ) +
                      cc::flag( toBool( sceneState ) ) );
    notifySceneStateEvent( sceneState );
}

void RepeatFunctionOperator::moveTimeWindow() {
    if( m_transitions.empty() )
        return;

    m_transitions.pop_front();
    if( !m_transitions.empty() && !toBool( m_transitions.front().m_currentState ) )
        m_transitions.pop_front();

    while( !m_transitions.empty() ) {
        const auto& transition = m_transitions.front();
        if( diffFromNow( transition.m_transition ) <= m_intervalMilliseconds )
            break;

        // remove old True transition
        m_transitions.pop_front();
        // remove corresponding to it False transition, no matter how far was it.
        if( !m_transitions.empty() && !toBool( m_transitions.front().m_currentState ) )
            m_transitions.pop_front();
    }
}

void RepeatFunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine