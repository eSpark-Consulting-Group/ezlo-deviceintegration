#include "scenes_manager/scenes_engine/operators/visitors/operator_name_visitor.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include <utility>

namespace SceneEngine {

using namespace Model::Resources;

void OperatorNameVisitor::visit( AbstractOperator& ) {
    set( "abstract" );
}

void OperatorNameVisitor::visit( EventOperator& ) {
    set( "event" );
}

void OperatorNameVisitor::visit( NotOperator& ) {
    set( OperatorNames::NotOperator );
}

void OperatorNameVisitor::visit( OrOperator& ) {
    set( OperatorNames::OrOperator );
}

void OperatorNameVisitor::visit( XorOperator& ) {
    set( OperatorNames::XorOperator );
}

void OperatorNameVisitor::visit( AndOperator& ) {
    set( OperatorNames::AndOperator );
}

void OperatorNameVisitor::visit( MultipleChildrenOperator& ) {
    set( "multiple_children" );
}

void OperatorNameVisitor::visit( FunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true );
}

void OperatorNameVisitor::visit( ForLessFunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true, FunctionFields::ForLess );
}

void OperatorNameVisitor::visit( ForAtLeastFunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true, FunctionFields::ForAtLeast );
}

void OperatorNameVisitor::visit( RepeatFunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true, FunctionFields::Repeat );
}

void OperatorNameVisitor::visit( FollowFunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true, FunctionFields::Follow );
}

void OperatorNameVisitor::visit( PulseFunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true, FunctionFields::Pulse );
}

void OperatorNameVisitor::visit( LatchFunctionOperator& ) {
    set( OperatorNames::FunctionOperator, true, FunctionFields::Latch );
}

const std::string& OperatorNameVisitor::name() const {
    return m_operatorName;
}

bool OperatorNameVisitor::isFunction() const {
    return m_isFunction;
}

const std::string& OperatorNameVisitor::functionName() const {
    return m_functionName;
}

void OperatorNameVisitor::clear() {
    m_operatorName.clear();
    m_isFunction = false;
    m_functionName.clear();
}

void OperatorNameVisitor::set( std::string name, bool isFunction, std::string functionName ) {
    clear();
    m_operatorName = std::move( name );
    m_isFunction = isFunction;
    m_functionName = std::move( functionName );
}

bool OperatorNameVisitor::isLatchInAndOperator( AbstractOperator& op ) {
    bool isLatchInAnd = false;

    OperatorNameVisitor nameVisitor;
    op.accept( nameVisitor );

    using namespace Model::Resources;
    if( nameVisitor.name() == OperatorNames::FunctionOperator && nameVisitor.isFunction() &&
        nameVisitor.functionName() == FunctionFields::Latch ) {
        if( op.getParent() ) {
            op.getParent()->accept( nameVisitor );

            isLatchInAnd = nameVisitor.name() == OperatorNames::AndOperator;
            if( isLatchInAnd ) {
                LOGGER_DEBUG( cc::debug( "Parent of this latch is an AND operator, blockId: " ) +
                              op.getParent()->getBlockID() );
            }
        }
    }
    return isLatchInAnd;
}

}  // namespace SceneEngine
