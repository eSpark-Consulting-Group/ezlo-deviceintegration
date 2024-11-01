// Extracted from scene_operator.cpp
#include "scenes_manager/scenes_engine/operators/not_operator.h"

namespace SceneEngine {

AbstractOperator::OperatorState NotOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !m_isEnabled || !m_child ) {
        return OperatorState::Disabled;
    }
    if( state == OperatorState::Disabled ) {
        return OperatorState::Disabled;
    }
    if( state == OperatorState::True ) {
        state = OperatorState::False;
    }
    else {
        state = OperatorState::True;
    }
    notifyOperatorStateEvent( state );
    return AbstractOperator::checkParent( child, state );
}

AbstractOperator::OperatorState NotOperator::checkChildren() {
    if( !m_isEnabled || !m_child ) {
        return OperatorState::Disabled;
    }
    auto state = m_child->checkChildren();
    if( state == OperatorState::Disabled ) {
        return OperatorState::Disabled;
    }
    if( state == OperatorState::True ) {
        state = OperatorState::False;
    }
    else {
        state = OperatorState::True;
    }
    return state;
}

void NotOperator::registerChild( SharedOperator child ) {
    if( child ) {
        if( isSceneStateEventHandlerDisabled() )
            child->setSceneStateEventHandlerDisabled( isSceneStateEventHandlerDisabled() );
        m_child = child;
    }
}

void NotOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

const AbstractOperator::SharedOperator& NotOperator::getChild() const {
    return m_child;
}

}  // namespace SceneEngine