#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class StringValue : public Value {
public:
    static std::unique_ptr< StringValue > createValue( const Json& value );
    static std::unique_ptr< StringValue > createDefaultValue();

    ~StringValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    const std::string& get() const;

    void set( const std::string& value );

protected:
    bool isLess( const Value& other ) const override;
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    StringValue();
    StringValue( std::string&& value );

private:
    std::string m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
