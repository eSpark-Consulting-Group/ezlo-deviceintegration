// Extracted from scene_operator.cpp
#include "scenes_manager/scenes_engine/operators/event_operator.h"

namespace SceneEngine {

AbstractOperator::OperatorState EventOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( m_isEnabled ) {
        return AbstractOperator::checkParent( child, state );
    }
    return OperatorState::Disabled;
}

AbstractOperator::OperatorState EventOperator::checkChildren() {
    auto state = ( m_isEnabled ) ? toOperatorState( m_state ) : OperatorState::Disabled;
    return state;
}

void EventOperator::registerChild( SharedOperator /*child*/ ) {}

void EventOperator::setState( bool state ) {
    m_state = state;
}

void EventOperator::notifyOperatorState() {
    if( isEnabled() )
        notifyOperatorStateEvent( toOperatorState( m_state ) );
}

AbstractOperator::OperatorState EventOperator::evaluateSubtree() {
    auto subtreeState = checkParent( shared_from_this(), toOperatorState( m_state ) );

    if( isEnabled() )
        notifySceneStateEvent( subtreeState );

    return subtreeState;
}

void EventOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine
