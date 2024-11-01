#pragma once

#include "cored/model/values/value_visitor.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ItemValueFloatVisitor : public CoreD::Model::ValueDefaultConstVisitor {
public:
    ItemValueFloatVisitor( const char* baseScale = nullptr );

    void visit( const HubPlatform::CoreD::Model::IntValue& intValue ) override;
    void visit( const HubPlatform::CoreD::Model::FloatValue& floatValue ) override;
    void visit( const HubPlatform::CoreD::Model::ScalableValue& itemValue ) override;
    float getValue() const;
    void clear();

private:
    float m_value = 0.0f;
    const char* m_baseScale = nullptr;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
