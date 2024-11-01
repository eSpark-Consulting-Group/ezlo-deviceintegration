#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class AssociationValue : public Value {
public:
    static std::unique_ptr< AssociationValue > createValue( const Json& value );
    static std::unique_ptr< AssociationValue > createDefaultValue();

    ~AssociationValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    using Value::Value;

    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    static constexpr int ms_invalidChannelId{ -1 };

    uint8_t m_targetNode{ 0 };
    int m_targetChannel{ ms_invalidChannelId };
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
