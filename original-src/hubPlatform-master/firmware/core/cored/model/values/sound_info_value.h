#pragma once

#include "cored/model/values/value.h"

#include "base_utils/enums/color.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class SoundInfoValue : public Value {
public:
    static std::unique_ptr< SoundInfoValue > createValue( const JsonObject& value );
    static std::unique_ptr< SoundInfoValue > createDefaultValue();

    ~SoundInfoValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    SoundInfoValue( std::string&& name, int duration );

private:
    std::string m_name;
    int m_duration;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
