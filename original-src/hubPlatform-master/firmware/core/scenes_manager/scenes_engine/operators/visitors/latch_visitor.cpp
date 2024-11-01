#include "scenes_manager/scenes_engine/operators/visitors/latch_visitor.h"
#include "scenes_manager/scenes_engine/operators/latch_function_operator.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

LatchVisitor::LatchVisitor( bool resetLatch )
: m_resetLatch( resetLatch ) {}

void LatchVisitor::visit( LatchFunctionOperator& op ) {
    m_isLatch = true;
    m_isLatchClosed = op.isLatched();

    if( m_resetLatch )
        resetLatch( op );
}

bool LatchVisitor::isLatch() const {
    return m_isLatch;
}

bool LatchVisitor::isLatchClosed() const {
    return m_isLatchClosed;
}

bool LatchVisitor::isResetLatch() const {
    return m_resetLatch;
}

bool LatchVisitor::isCompleted() const {
    return m_isCompleted;
}

void LatchVisitor::clear() {
    m_isCompleted = false;
    m_isLatch = false;
    m_isLatchClosed = false;
}

void LatchVisitor::resetLatch( LatchFunctionOperator& op ) {
    if( op.isLatched() ) {
        LOGGER_DEBUG_RAW( cc::debug( "LatchVisitor::resetLatch is called on latched block, id: " ) + op.getBlockID() );
        op.openLatch();
        m_isCompleted = true;
        m_isLatchClosed = op.isLatched();
    }
}

}  // namespace SceneEngine
