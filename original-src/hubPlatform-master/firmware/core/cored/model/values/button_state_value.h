#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ButtonStateValue : public Value {
public:
    static std::unique_ptr< ButtonStateValue > createValue( const Json& value );
    static std::unique_ptr< ButtonStateValue > createDefaultValue();

    ~ButtonStateValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    ButtonStateValue( int buttonId, std::string buttonState );

private:
    int m_buttonId;
    std::string m_buttonState;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
