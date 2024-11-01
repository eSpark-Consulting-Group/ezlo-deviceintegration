#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class LedEffectValue : public Value {
public:
    static std::unique_ptr< LedEffectValue > createValue( const Json& value );
    static std::unique_ptr< LedEffectValue > createDefaultValue();

    ~LedEffectValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    LedEffectValue( std::string effectName, std::string effectDescription, std::string effectHeadpieceUrl );

private:
    std::string m_effectName;
    std::string m_effectDescription;
    std::string m_effectHeadpieceUrl;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
