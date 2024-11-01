#pragma once
// Extracted from scene_operator.h
#include "scenes_manager/scenes_engine/operators/scene_operator.h"

namespace SceneEngine {
namespace UT {
class TransactionalEventsUpdaterTest;
}  // namespace UT

class EventOperator : public AbstractOperator {
public:
    OperatorState checkParent( SharedOperator child, OperatorState state ) override;
    OperatorState checkChildren() override;
    void registerChild( SharedOperator child ) override;
    void accept( OperatorVisitor& visitor ) override;

    void setState( bool state );
    void notifyOperatorState();
    AbstractOperator::OperatorState evaluateSubtree();

private:
    bool m_state{ false };

    friend class UT::TransactionalEventsUpdaterTest;
};

}  // namespace SceneEngine
