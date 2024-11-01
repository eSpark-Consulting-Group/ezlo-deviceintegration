#pragma once
#include "scenes_manager/scenes_engine/operators/and_operator.h"

namespace SceneEngine {

class FunctionOperator : public AndOperator {
public:
    FunctionOperator() = default;

    void registerChild( SharedOperator child ) override;
    void accept( OperatorVisitor& visitor ) override;

    void setInterceptSceneState( bool value );
    bool getInterceptSceneState() const;

private:
    bool m_interceptSceneState{ false };
};

}  // namespace SceneEngine
