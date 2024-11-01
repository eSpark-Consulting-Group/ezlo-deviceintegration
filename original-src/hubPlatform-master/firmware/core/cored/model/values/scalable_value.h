#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ScalableValue;
using ScalableValueUPtr = std::unique_ptr< ScalableValue >;

class ScalableValue : public Value {
public:
    static ScalableValueUPtr createValue( ValueType type, const Json& value );
    static ScalableValueUPtr createDefaultValue( ValueType type );

    ~ScalableValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    void setValue( float value );

    float getValue() const;
    float getConvertedValue( ValueScale otherScale ) const;
    ValueScale getScale() const;
    bool isRelative() const;

protected:
    bool isLess( const Value& other ) const override;
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    ScalableValue( ValueType type, float value, ValueScale scale, bool isRelative );

private:
    float m_value;
    ValueScale m_scale;
    bool m_isRelative;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
