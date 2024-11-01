// Extracted from scene_operator.cpp
#include "scenes_manager/scenes_engine/operators/and_operator.h"

namespace SceneEngine {

AbstractOperator::OperatorState AndOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }
    for( const auto& condition : getChildren() ) {
        if( condition == child ) {
            if( state == OperatorState::False ) {
                notifyOperatorStateEvent( OperatorState::False );
                return AbstractOperator::checkParent( child, OperatorState::False );
            }
            continue;
        }
        if( condition->checkChildren() == OperatorState::False ) {
            notifyOperatorStateEvent( OperatorState::False );
            return AbstractOperator::checkParent( child, OperatorState::False );
        }
    }
    notifyOperatorStateEvent( OperatorState::True );
    return AbstractOperator::checkParent( child, OperatorState::True );
}

AbstractOperator::OperatorState AndOperator::checkChildren() {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }
    std::size_t count = 0;
    for( const auto& condition : getChildren() ) {
        auto state = condition->checkChildren();
        if( state == OperatorState::False ) {
            return OperatorState::False;
        }
        else if( state == OperatorState::Disabled ) {
            ++count;
        }
    }
    if( getChildren().size() == count ) {
        return OperatorState::Disabled;
    }
    return OperatorState::True;
}

void AndOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine