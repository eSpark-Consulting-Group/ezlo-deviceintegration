#pragma once

#include "cored/model/values/value.h"
#include "cored/model/setting/synchronization_status.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ZwaveConfigurationValue : public Value {
public:
    static std::unique_ptr< ZwaveConfigurationValue > createValue( const Json& value );
    static std::unique_ptr< ZwaveConfigurationValue > createDefaultValue();

    ~ZwaveConfigurationValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    using Value::Value;

    enum class Format { Decimal, Hexadecimal };

    static Format getFormat( const Json& value );
    static std::string formatToString( Format value );

private:
    std::string m_name;
    std::string m_description;
    uint8_t m_number{ 0 };
    uint8_t m_size{ 0 };
    bool m_hasSetter{ true };
    uint32_t m_value{ 0 };
    Format m_format{ Format::Decimal };
    SynchronizationStatus::Enum m_status{ SynchronizationStatus::Pending };
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
