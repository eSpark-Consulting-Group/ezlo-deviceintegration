#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ShutterSecondPartValue : public Value {
public:
    static std::unique_ptr< ShutterSecondPartValue > createValue( const Json& value );
    static std::unique_ptr< ShutterSecondPartValue > createDefaultValue();

    ~ShutterSecondPartValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    ShutterSecondPartValue( std::string deviceId, std::string partFunction );

private:
    std::string m_deviceId;
    std::string m_partFunction;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
