#pragma once
// Extracted from scene_operator.h
#include "scenes_manager/scenes_engine/operators/scene_operator.h"

namespace SceneEngine {

class NotOperator : public AbstractOperator {
public:
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void registerChild( SharedOperator child ) override;
    void accept( OperatorVisitor& visitor ) override;

    const SharedOperator& getChild() const;

private:
    SharedOperator m_child;
};

}  // namespace SceneEngine
