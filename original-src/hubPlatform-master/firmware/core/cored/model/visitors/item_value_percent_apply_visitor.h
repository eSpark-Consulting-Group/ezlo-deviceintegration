#pragma once

#include "cored/model/values/value_visitor.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ItemValuePercentApplyVisitor : public ValueDefaultVisitor {
public:
    using Value = CoreD::Model::Value;

    ItemValuePercentApplyVisitor( Value& minValue, Value& maxValue, double percent );

    ItemValuePercentApplyVisitor( ItemValuePercentApplyVisitor const& ) = delete;

    ItemValuePercentApplyVisitor& operator=( ItemValuePercentApplyVisitor const& ) = delete;

    void visit( IntValue& intValue ) override;
    void visit( FloatValue& floatValue ) override;
    void visit( ScalableValue& itemValue ) override;
    void visit( Value& itemValue );

private:
    Value& m_minValue;
    Value& m_maxValue;
    double m_percent = 0.0f;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform