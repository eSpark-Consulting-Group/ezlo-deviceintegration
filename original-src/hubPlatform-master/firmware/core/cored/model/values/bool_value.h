#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class BoolValue : public Value {
public:
    static std::unique_ptr< BoolValue > createValue( const Json& value );
    static std::unique_ptr< BoolValue > createDefaultValue();

    ~BoolValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    bool get() const;

    void set( bool value );

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    BoolValue( bool value );

private:
    bool m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
