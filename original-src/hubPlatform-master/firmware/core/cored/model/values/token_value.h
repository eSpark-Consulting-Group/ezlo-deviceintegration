#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class TokenValue : public Value {
public:
    static std::unique_ptr< TokenValue > createValue( const Json& value );
    static std::unique_ptr< TokenValue > createDefaultValue();

    ~TokenValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    const std::string& get() const;

    void set( const std::string& value );

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    TokenValue();
    explicit TokenValue( std::string&& value );

private:
    std::string m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
