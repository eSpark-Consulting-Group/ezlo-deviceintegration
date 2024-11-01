#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class UserCodeValue : public Value {
public:
    static std::unique_ptr< UserCodeValue > createValue( const Json& value );
    static std::unique_ptr< UserCodeValue > createDefaultValue();

    ~UserCodeValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    const std::string& getUserCode() const;
    void setUserCode( const std::string& code );

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    UserCodeValue( std::string userName, std::string userCode, std::string mode );

private:
    std::string m_userName;
    std::string m_userCode;
    std::string m_userMode;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
