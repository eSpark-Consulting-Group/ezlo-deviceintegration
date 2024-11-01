#pragma once

#include <memory>

namespace HubPlatform {
namespace CoreD {
namespace Model {
class Value;
class ValueType;
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform

namespace SceneEngine {

class ComparableValue {
    using ValueUPtr = std::unique_ptr< HubPlatform::CoreD::Model::Value >;
    enum class Type { Float = 1, String = 2 };

public:
    ComparableValue( const HubPlatform::CoreD::Model::Value& value );

    bool operator==( const ComparableValue& other ) const;
    bool operator<( const ComparableValue& other ) const;

    static bool isNumericType( const HubPlatform::CoreD::Model::ValueType& valueType );
    static bool isStringType( const HubPlatform::CoreD::Model::ValueType& valueType );

private:
    Type m_type = Type::Float;

    float m_valueFloat;
    std::string m_valueString;
};

}  // namespace SceneEngine
