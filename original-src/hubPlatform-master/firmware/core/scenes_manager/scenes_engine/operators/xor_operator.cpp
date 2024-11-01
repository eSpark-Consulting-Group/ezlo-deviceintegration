#include "scenes_manager/scenes_engine/operators/xor_operator.h"

namespace SceneEngine {

AbstractOperator::OperatorState XorOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !m_isEnabled || getChildren().size() < 2 ) {
        return OperatorState::Disabled;
    }
    unsigned trueCount = 0;
    for( const auto& condition : getChildren() ) {
        OperatorState loopState = condition->checkChildren();
        if( loopState == OperatorState::True )
            trueCount++;
    }
    OperatorState childrenState = toOperatorState( trueCount % 2 == 1 );
    notifyOperatorStateEvent( childrenState );
    return AbstractOperator::checkParent( child, childrenState );
}

/** The XOR condition should be TRUE in case of number of True values is odd, FALSE otherwise */
AbstractOperator::OperatorState XorOperator::checkChildren() {
    if( !m_isEnabled || getChildren().size() < 2 ) {
        return OperatorState::Disabled;
    }
    unsigned disabledCount = 0;
    unsigned trueCount = 0;

    for( const auto& condition : getChildren() ) {
        auto state = condition->checkChildren();

        switch( state ) {
        case OperatorState::Disabled:
            ++disabledCount;
            break;
        case OperatorState::True:
            ++trueCount;
            break;
        default:;
        }
    }
    if( getChildren().size() == disabledCount ) {
        return OperatorState::Disabled;
    }
    auto state = toOperatorState( trueCount % 2 == 1 );
    return state;
}

void XorOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine