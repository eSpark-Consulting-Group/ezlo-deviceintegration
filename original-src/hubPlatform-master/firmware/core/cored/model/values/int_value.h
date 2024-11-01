#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class IntValue : public Value {
public:
    static std::unique_ptr< IntValue > createValue( const Json& value );
    static std::unique_ptr< IntValue > createDefaultValue();

    ~IntValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    int get() const;

    void set( int value );

protected:
    bool isLess( const Value& other ) const override;
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    IntValue( int value );

private:
    int m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
