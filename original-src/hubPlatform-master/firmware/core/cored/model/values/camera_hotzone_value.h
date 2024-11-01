#pragma once
#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class CameraHotzoneValue : public Value {
public:
    static std::unique_ptr< CameraHotzoneValue > createValue( const Json& value );
    static std::unique_ptr< CameraHotzoneValue > createDefaultValue();

    ~CameraHotzoneValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    static bool isKnownType( const std::string& type );

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    CameraHotzoneValue( Json value );

private:
    Json m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
