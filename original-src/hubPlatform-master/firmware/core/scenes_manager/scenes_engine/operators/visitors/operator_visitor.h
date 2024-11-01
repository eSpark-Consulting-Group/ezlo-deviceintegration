#pragma once

namespace SceneEngine {

class AbstractOperator;
class EventOperator;
class NotOperator;
class OrOperator;
class XorOperator;
class AndOperator;
class MultipleChildrenOperator;
class FunctionOperator;
class ForLessFunctionOperator;
class ForAtLeastFunctionOperator;
class RepeatFunctionOperator;
class FollowFunctionOperator;
class PulseFunctionOperator;
class LatchFunctionOperator;

class OperatorVisitor {
public:
    virtual ~OperatorVisitor() = default;
    virtual void visit( EventOperator& ) = 0;
    virtual void visit( NotOperator& ) = 0;
    virtual void visit( OrOperator& ) = 0;
    virtual void visit( XorOperator& ) = 0;
    virtual void visit( AndOperator& ) = 0;
    virtual void visit( MultipleChildrenOperator& ) = 0;
    virtual void visit( FunctionOperator& ) = 0;
    virtual void visit( ForLessFunctionOperator& ) = 0;
    virtual void visit( ForAtLeastFunctionOperator& ) = 0;
    virtual void visit( RepeatFunctionOperator& ) = 0;
    virtual void visit( FollowFunctionOperator& ) = 0;
    virtual void visit( PulseFunctionOperator& ) = 0;
    virtual void visit( LatchFunctionOperator& ) = 0;
};

class DefaultOperatorVisitor : public OperatorVisitor {
public:
    void visit( EventOperator& ) override {}
    void visit( NotOperator& ) override {}
    void visit( OrOperator& ) override {}
    void visit( XorOperator& ) override {}
    void visit( AndOperator& ) override {}
    void visit( MultipleChildrenOperator& ) override {}
    void visit( FunctionOperator& ) override {}
    void visit( ForLessFunctionOperator& ) override {}
    void visit( ForAtLeastFunctionOperator& ) override {}
    void visit( RepeatFunctionOperator& ) override {}
    void visit( FollowFunctionOperator& ) override {}
    void visit( PulseFunctionOperator& ) override {}
    void visit( LatchFunctionOperator& ) override {}
};

}  // namespace SceneEngine