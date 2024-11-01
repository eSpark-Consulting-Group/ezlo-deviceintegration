#pragma once
// Extracted from scene_operator.h
#include "scenes_manager/scenes_engine/operators/scene_operator.h"

namespace SceneEngine {

class MultipleChildrenOperator : public AbstractOperator {
public:
    void registerChild( SharedOperator child ) override;
    void accept( OperatorVisitor& visitor ) override;

    void forEach( const std::function< void( AbstractOperator& ) >& visit ) const;

protected:
    std::vector< SharedOperator >& getChildren();
    const std::vector< SharedOperator >& getChildren() const;

protected:
    std::vector< SharedOperator > m_children;
};

}  // namespace SceneEngine
