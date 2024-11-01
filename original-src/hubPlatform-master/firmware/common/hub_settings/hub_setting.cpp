#include "hub_settings/hub_setting.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Hub {

Setting::Setting( Json json )
: m_json( std::move( json ) ) {
    if( m_json.count( "_id" ) != 0 ) {
        m_json[ "name" ] = m_json[ "_id" ];
        m_json.erase( "_id" );
    }
}

bool Setting::isValid() const {
    if( m_json.count( "name" ) == 0 )
        return false;
    if( m_json.count( "valueType" ) == 0 )
        return false;
    if( m_json.count( "value" ) == 0 )
        return false;

    auto value = m_json.find( "value" ).value();

    switch( getValueType() ) {
    case ValueType::Bool: {
        if( !value.is_boolean() )
            return false;
        break;
    }
    case ValueType::String: {
        if( !value.is_string() )
            return false;
        break;
    }
    case ValueType::Token: {
        if( m_json.count( "enum" ) == 0 )
            return false;

        if( !value.is_string() )
            return false;

        auto values = getPossibleTokenValues();
        if( std::find( values.begin(), values.end(), value.get< std::string >() ) == values.end() ) {
            return false;
        }
        break;
    }
    case ValueType::Int: {
        if( !value.is_number_integer() )
            return false;
        break;
    }
    }

    return true;
}

std::string Setting::getName() const {
    return json_get_str( m_json, "name" );
}

Setting::ValueType Setting::getValueType() const {
    return valueTypeFromString( json_get_str( m_json, "valueType" ) );
}

std::string Setting::getTokenValue() const {
    return json_get_str( m_json, "value" );
}

std::string Setting::getStringValue() const {
    return json_get_str( m_json, "value" );
}

bool Setting::getBoolValue() const {
    return json_get_bool( m_json, "value" );
}

int Setting::getIntValue() const {
    return json_get_int( m_json, "value", 0 );
}

Json Setting::getValue() const {
    return m_json[ "value" ];
}

Setting::PossibleValues Setting::getPossibleTokenValues() const {
    return json_get_vec< std::string >( m_json, "enum" );
}

bool Setting::setBoolValue( bool value ) {
    if( getValueType() != ValueType::Bool )
        return false;

    m_json[ "value" ] = value;
    return true;
}

bool Setting::setStringValue( const std::string& value ) {
    if( getValueType() != ValueType::String )
        return false;

    m_json[ "value" ] = value;
    return true;
}

bool Setting::setTokenValue( const std::string& value ) {
    if( getValueType() != ValueType::Token )
        return false;

    auto values = getPossibleTokenValues();
    if( std::find( values.begin(), values.end(), value ) == values.end() ) {
        return false;
    }

    m_json[ "value" ] = value;
    return true;
}

bool Setting::setIntValue( int value ) {
    if( getValueType() != ValueType::Int )
        return false;

    m_json[ "value" ] = value;
    return true;
}

bool Setting::setValue( const Json& value ) {
    switch( getValueType() ) {
    case ValueType::Bool:
        if( value.is_boolean() ) {
            return setBoolValue( value.get< bool >() );
        }
        break;
    case ValueType::String:
        if( value.is_string() ) {
            return setStringValue( value.get< std::string >() );
        }
        break;
    case ValueType::Token:
        if( value.is_string() ) {
            return setTokenValue( value.get< std::string >() );
        }
        break;
    case ValueType::Int:
        if( value.is_number_integer() ) {
            return setIntValue( value.get< int >() );
        }
        break;
    }
    return false;
}

const Json& Setting::asJson() const {
    return m_json;
}

Setting::ValueType Setting::valueTypeFromString( const std::string& valueType ) {
    if( valueType == "token" ) {
        return ValueType::Token;
    }
    if( valueType == "string" ) {
        return ValueType::String;
    }
    else if( valueType == "bool" ) {
        return ValueType::Bool;
    }
    else if( valueType == "int" ) {
        return ValueType::Int;
    }
    return ValueType::Bool;
}

}  // namespace Hub
}  // namespace HubPlatform
