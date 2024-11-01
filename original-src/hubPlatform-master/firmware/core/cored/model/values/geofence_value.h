#pragma once
#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class GeofenceValue : public Value {
public:
    static std::unique_ptr< GeofenceValue > createValue( const Json& value );
    static std::unique_ptr< GeofenceValue > createDefaultValue();

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    GeofenceValue( int modifiedTime );

private:
    int m_modifiedTime = 0;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
