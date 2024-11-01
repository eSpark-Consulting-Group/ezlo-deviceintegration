#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class UserCodeActionValue : public Value {
public:
    static std::unique_ptr< UserCodeActionValue > createValue( const Json& value );
    static std::unique_ptr< UserCodeActionValue > createDefaultValue();

    ~UserCodeActionValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    UserCodeActionValue( int userId, std::string action );

private:
    int m_userId;
    std::string m_action;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
