#pragma once
#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class DetectableObjectBase : public Value {
public:
    static std::unique_ptr< DetectableObjectBase > createDefaultValue( ValueType::Enum type );

    virtual ~DetectableObjectBase() override;

    Json toJson() const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

protected:
    static void validateValue( const Json& value, ValueType::Enum type );
    static Json getDefaultValue( ValueType::Enum type );

protected:
    DetectableObjectBase( Json value, ValueType::Enum type );

private:
    Json m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
