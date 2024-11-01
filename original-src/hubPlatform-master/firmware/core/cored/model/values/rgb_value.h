#pragma once

#include "cored/model/values/value.h"

#include "base_utils/enums/color.h"

#include <map>
#include <functional>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class RgbValue : public Value {
public:
    static std::unique_ptr< RgbValue > createValue( const JsonObject& value );
    static std::unique_ptr< RgbValue > createDefaultValue();

    ~RgbValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    bool forEachComponent( std::function< bool( Color, uint8_t ) > _callBack ) const;

    bool update( Color componentId, uint8_t value );

    uint8_t getColor( Color color ) const;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    RgbValue();

    void setComponentValue( Color componentId, uint8_t value );

private:
    using Components = std::map< Color, uint8_t >;
    Components m_components;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
