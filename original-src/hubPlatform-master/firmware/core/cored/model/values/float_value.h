#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class FloatValue : public Value {
public:
    static std::unique_ptr< FloatValue > createValue( const Json& value );
    static std::unique_ptr< FloatValue > createDefaultValue();

    ~FloatValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    float get() const;

    void set( float value );

protected:
    bool isLess( const Value& other ) const override;
    bool isEqual( const Value& value ) const override;
    void set( const Value& other ) override;

private:
    FloatValue( float value );

private:
    float m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
