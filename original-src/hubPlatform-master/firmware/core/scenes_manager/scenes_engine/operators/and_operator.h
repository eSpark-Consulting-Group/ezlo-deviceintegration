#pragma once
// Extracted from scene_operator.h
#include "scenes_manager/scenes_engine/operators/multiple_children_operator.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"

namespace SceneEngine {

class AndOperator : public MultipleChildrenOperator {
public:
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void accept( OperatorVisitor& visitor ) override;
};

}  // namespace SceneEngine
