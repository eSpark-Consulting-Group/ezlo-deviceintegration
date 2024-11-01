#include "scenes_manager/scenes_engine/operators/latch_function_operator.h"
#include "scenes_manager/scenes_engine/operators/operator_exceptions.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

LatchFunctionOperator::LatchFunctionOperator( bool )
: m_isLatched( false ) {
    setInterceptSceneState( true );
}

void LatchFunctionOperator::registerChild( AbstractOperator::SharedOperator child ) {
    if( !child )
        return;
    if( !getChildren().empty() ) {
        throw LatchException( "Only one child can be registered" );
    }
    FunctionOperator::registerChild( child );
}

AbstractOperator::OperatorState LatchFunctionOperator::checkParent( AbstractOperator::SharedOperator child,
                                                                    AbstractOperator::OperatorState state ) {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }
    if( isLatched() ) {
        LOGGER_DEBUG_RAW( cc::debug( "Latch is closed, ignore signals, return true, blockId: " ) + getBlockID() );
        return AbstractOperator::OperatorState::True;
    }

    auto parentOf = []( AbstractOperator& parent, AbstractOperator& child ) -> bool {
        auto parentOfAChild = child.getParent();
        while( parentOfAChild != nullptr ) {
            if( &parent == parentOfAChild.get() )
                return true;
        }
        return false;
    };

    for( const auto& kid : this->getChildren() ) {
        if( kid == child || parentOf( *kid, *child ) ) {
            bool childIsTrue = AbstractOperator::toBool( state );
            if( childIsTrue ) {
                if( m_currentState != state ) {
                    saveState( state );
                    closeLatch();

                    auto sceneState = FunctionOperator::checkParent( shared_from_this(), state );
                    notifySceneStateEvent( sceneState );
                }
            }
            else {
                if( m_currentState != state ) {
                    saveState( state );
                    notifyOperatorStateEvent( state );
                }
            }
            return FunctionOperator::checkParent( child, state );
        }
    }
    return FunctionOperator::checkParent( child, state );
}

AbstractOperator::OperatorState LatchFunctionOperator::checkChildren() {
    if( isLatched() )
        return AbstractOperator::OperatorState::True;
    return FunctionOperator::checkChildren();
}

AbstractOperator::OperatorState LatchFunctionOperator::saveState( AbstractOperator::OperatorState newState ) {
    auto oldState = m_currentState;
    if( oldState != newState ) {
        m_currentState = newState;
    }
    return oldState;
}

/** When the latch is closed, all signals from children are ignored. */
void LatchFunctionOperator::closeLatch() {
    Log::info( cc::info( "latch is closed, blockId: " ) + getBlockID() );
    m_isLatched = true;
    notifyOperatorStateEvent( AbstractOperator::OperatorState::True );
}

/** Open the latch to allow signals from children to pass again */
void LatchFunctionOperator::openLatch() {
    Log::info( cc::info( "Latch is opened, notify operator state False, blockId: " ) + getBlockID() );
    m_isLatched = false;
    saveState( AbstractOperator::OperatorState::False );
    notifyOperatorStateEvent( AbstractOperator::OperatorState::False );
}

bool LatchFunctionOperator::isLatched() const {
    return m_isLatched;
}

void LatchFunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine
