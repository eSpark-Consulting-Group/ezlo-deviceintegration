#include "scenes_manager/scenes_engine/events/comparable_value.h"

#include "scenes_manager/scenes_engine/events/event_exception.h"
#include "cored/model/values/value.h"
#include "cored/model/values/value_type.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/model/visitors/item_value_string_visitor.h"
#include "base_utils/float_operations.h"

namespace SceneEngine {

ComparableValue::ComparableValue( const HubPlatform::CoreD::Model::Value& value ) {
    using namespace HubPlatform::CoreD::Model;
    try {
        const auto& valueType = value.getType();
        if( isNumericType( valueType ) ) {
            ItemValueFloatVisitor floatVisitor;
            value.accept( floatVisitor );
            m_valueFloat = floatVisitor.getValue();
            m_type = Type::Float;
        }
        else if( isStringType( valueType ) ) {
            ItemValueStringVisitor stringVisitor;
            value.accept( stringVisitor );
            m_valueString = stringVisitor.getValue();
            m_type = Type::String;
        }
        else {
            throw EventException( "Value cannot be converted to comparable value: unsupported value type" );
        }
    }
    catch( const std::exception& e ) {
        throw EventException( "Cannot convert value to comparable value: " + std::string( e.what() ) );
    }
}

bool ComparableValue::operator==( const ComparableValue& other ) const {
    using namespace HubPlatform::CoreD::Model;

    if( m_type == Type::Float && other.m_type == Type::Float )
        return HubPlatform::areFloatsEqual( m_valueFloat, other.m_valueFloat );
    else if( m_type == Type::String && other.m_type == Type::String )
        return m_valueString == other.m_valueString;

    throw EventException( "Operator == for particular comparable types is not defined" );
}

bool ComparableValue::operator<( const ComparableValue& other ) const {
    using namespace HubPlatform::CoreD::Model;

    if( m_type == Type::Float && other.m_type == Type::Float )
        return HubPlatform::isFloatLess( m_valueFloat, other.m_valueFloat );
    else if( m_type == Type::String && other.m_type == Type::String )
        return m_valueString < other.m_valueString;

    throw EventException( "Operator < for particular comparable types is not defined" );
}

bool ComparableValue::isNumericType( const HubPlatform::CoreD::Model::ValueType& valueType ) {
    using namespace HubPlatform::CoreD::Model;
    // NOTE: bool should be treated as standalone type

    if( valueType == ValueType::Enum::Int )
        return true;

    if( valueType == ValueType::Enum::Float )
        return true;

    if( valueType.getScales().size() )  // scalable value
        return true;

    return false;
}

bool ComparableValue::isStringType( const HubPlatform::CoreD::Model::ValueType& valueType ) {
    using namespace HubPlatform::CoreD::Model;
    return ( valueType == ValueType::Enum::String || valueType == ValueType::Enum::Token );
}

}  // namespace SceneEngine
