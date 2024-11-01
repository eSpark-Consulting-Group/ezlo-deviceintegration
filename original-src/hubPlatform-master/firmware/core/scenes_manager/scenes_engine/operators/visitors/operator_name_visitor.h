#pragma once

#include "scenes_manager/scenes_engine/operators/visitors/operator_visitor.h"
#include <string>

namespace SceneEngine {

/** OperatorName: gets name of an operator, function name if any. */
class OperatorNameVisitor final : public DefaultOperatorVisitor {
public:
    explicit OperatorNameVisitor() = default;

    void visit( AbstractOperator& );
    void visit( EventOperator& ) override;
    void visit( NotOperator& ) override;
    void visit( OrOperator& ) override;
    void visit( XorOperator& ) override;
    void visit( AndOperator& ) override;
    void visit( MultipleChildrenOperator& ) override;
    void visit( FunctionOperator& ) override;
    void visit( ForLessFunctionOperator& ) override;
    void visit( ForAtLeastFunctionOperator& ) override;
    void visit( RepeatFunctionOperator& ) override;
    void visit( FollowFunctionOperator& ) override;
    void visit( PulseFunctionOperator& ) override;
    void visit( LatchFunctionOperator& ) override;

    const std::string& name() const;
    bool isFunction() const;
    const std::string& functionName() const;

    static bool isLatchInAndOperator( AbstractOperator& op );

private:
    void clear();
    void set( std::string name, bool isFunction = false, std::string functionName = "" );

private:
    std::string m_operatorName;
    bool m_isFunction{ false };
    std::string m_functionName;
};

}  // namespace SceneEngine