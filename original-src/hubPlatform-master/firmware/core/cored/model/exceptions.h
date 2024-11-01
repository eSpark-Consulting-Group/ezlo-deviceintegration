#pragma once

#include "cored/model/values/value_type.h"

#include "json_utils/json_fwd.h"

#include <stdexcept>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class CoreDModelError : public std::runtime_error {
public:
    explicit CoreDModelError( const std::string& description )
    : runtime_error{ description } {}
};

class StringIsTooLongError : public CoreDModelError {
public:
    explicit StringIsTooLongError( const std::string& fieldName, uint32_t maxLength )
    : CoreDModelError{ "Field '" + fieldName + "' is too long. Maximum length is " + std::to_string( maxLength ) +
                       " symbols." } {}
};

class UnexpectedJsonValueFormatError : public CoreDModelError {
public:
    explicit UnexpectedJsonValueFormatError( const Json& actualValue, const ValueType& expectedType )
    : CoreDModelError( std::string( "Unexpected Json format for value type '" ) + expectedType.toString() +
                       "': " + actualValue.dump() ) {}

    explicit UnexpectedJsonValueFormatError( const Json& actualValue,
                                             const ValueType& expectedType,
                                             std::string reason )
    : CoreDModelError( std::string( "Unexpected Json format for value type '" ) + expectedType.toString() +
                       "', reason: " + std::move( reason ) + " : " + actualValue.dump() ) {}
};

class UnexpectedValueType : public CoreDModelError {
public:
    explicit UnexpectedValueType( ValueType actualType )
    : CoreDModelError( std::string( "Unexpected value type '" ) + getType( actualType ) + "'" ) {}

private:
    static std::string getType( const ValueType& actualType ) {
        auto name = actualType.toString();
        return name.empty() ? std::to_string( static_cast< typename std::underlying_type< ValueType::Enum >::type >(
                                  actualType.getMainType() ) ) :
                              name;
    }
};

class ScalesNotFoundError : public CoreDModelError {
public:
    explicit ScalesNotFoundError( const ValueType& type )
    : CoreDModelError( std::string( "Scales not found for value type '" ) + type.toString() + "'" ) {}
};

class InvalidValueScale : public CoreDModelError {
public:
    explicit InvalidValueScale( const ValueType& type, const char* scale = "" )
    : CoreDModelError( std::string( "Item scale cannot be converted to target scale '" ) + type.toString() +
                       "', scale: " + std::string( scale ? scale : "" ) ) {}
};

class ValueError : public CoreDModelError {
public:
    ValueError( const std::string& description )
    : CoreDModelError( description ) {}
};

class ItemDictionaryIdGenerationError : public CoreDModelError {
public:
    explicit ItemDictionaryIdGenerationError()
    : CoreDModelError( std::string( "Cannot generate item id" ) ) {}
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
