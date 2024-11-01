// Extracted from scene_operator.cpp
#include "scenes_manager/scenes_engine/operators/multiple_children_operator.h"

namespace SceneEngine {

void MultipleChildrenOperator::registerChild( AbstractOperator::SharedOperator child ) {
    if( child ) {
        if( isSceneStateEventHandlerDisabled() )
            child->setSceneStateEventHandlerDisabled( isSceneStateEventHandlerDisabled() );
        m_children.push_back( child );
    }
}

std::vector< AbstractOperator::SharedOperator >& MultipleChildrenOperator::getChildren() {
    return m_children;
}

const std::vector< AbstractOperator::SharedOperator >& MultipleChildrenOperator::getChildren() const {
    return m_children;
}

void MultipleChildrenOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

void MultipleChildrenOperator::forEach( const std::function< void( AbstractOperator& ) >& visit ) const {
    if( visit != nullptr ) {
        for( auto& op : getChildren() ) {
            visit( *op );
        }
    }
}

}  // namespace SceneEngine