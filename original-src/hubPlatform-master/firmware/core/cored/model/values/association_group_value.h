#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class AssociationGroupValue : public Value {
public:
    static std::unique_ptr< AssociationGroupValue > createValue( const Json& value );
    static std::unique_ptr< AssociationGroupValue > createDefaultValue();

    ~AssociationGroupValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    using Value::Value;

    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    std::string m_name;
    std::string m_profile;
    int m_channel{ 0 };
    int m_maxAssociationsNumber{ 0 };
    std::vector< std::string > m_commands;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
