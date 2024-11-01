#pragma once

#include "scenes_manager/scenes_engine/operators/visitors/operator_visitor.h"
#include <string>

namespace SceneEngine {

/** LatchVisitor: defines if an operator is a latch function and resets the latch if any */
class LatchVisitor final : public DefaultOperatorVisitor {
public:
    explicit LatchVisitor( bool resetLatch = false );
    void visit( LatchFunctionOperator& op ) override;

    bool isLatch() const;
    bool isLatchClosed() const;
    bool isResetLatch() const;
    bool isCompleted() const;
    void clear();

private:
    void resetLatch( LatchFunctionOperator& op );

private:
    bool m_resetLatch{ false };
    mutable bool m_isLatch{ false };
    mutable bool m_isLatchClosed{ false };
    mutable bool m_isCompleted{ false };
};

}  // namespace SceneEngine