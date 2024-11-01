#include "scenes_manager/scenes_engine/operators/function_operator.h"

namespace SceneEngine {

void FunctionOperator::registerChild( AbstractOperator::SharedOperator child ) {
    if( !child )
        return;
    if( getInterceptSceneState() ) {
        child->setSceneStateEventHandlerDisabled( true );
    }
    MultipleChildrenOperator::registerChild( child );
}

void FunctionOperator::setInterceptSceneState( bool value ) {
    m_interceptSceneState = value;
}

bool FunctionOperator::getInterceptSceneState() const {
    return m_interceptSceneState;
}

void FunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine