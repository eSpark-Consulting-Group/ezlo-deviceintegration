#pragma once

#include "cored/model/values/value.h"
#include "cored/model/setting/text_element.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ActionValue : public Value {
public:
    static std::unique_ptr< ActionValue > createValue( const Json& value );
    static std::unique_ptr< ActionValue > createDefaultValue();

    ~ActionValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    using Value::Value;

private:
    TextElement m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
