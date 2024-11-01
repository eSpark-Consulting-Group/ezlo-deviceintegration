// Extracted from scene_operator.cpp
#include "scenes_manager/scenes_engine/operators/or_operator.h"

namespace SceneEngine {

AbstractOperator::OperatorState OrOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }
    for( const auto& condition : getChildren() ) {
        if( condition == child ) {
            if( state == OperatorState::True ) {
                notifyOperatorStateEvent( OperatorState::True );
                return AbstractOperator::checkParent( child, OperatorState::True );
            }
            continue;
        }
        if( condition->checkChildren() == OperatorState::True ) {
            notifyOperatorStateEvent( OperatorState::True );
            return AbstractOperator::checkParent( child, OperatorState::True );
        }
    }
    notifyOperatorStateEvent( OperatorState::False );
    return AbstractOperator::checkParent( child, OperatorState::False );
}

AbstractOperator::OperatorState OrOperator::checkChildren() {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }
    std::size_t count = 0;
    for( const auto& condition : getChildren() ) {
        auto state = condition->checkChildren();
        if( state == OperatorState::True ) {
            return OperatorState::True;
        }
        else if( state == OperatorState::Disabled ) {
            ++count;
        }
    }
    if( getChildren().size() == count ) {
        return OperatorState::Disabled;
    }
    return OperatorState::False;
}

void OrOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine