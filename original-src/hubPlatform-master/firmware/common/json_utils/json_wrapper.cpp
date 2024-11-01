#include "json_wrapper.h"

std::string dump( const Json& jsonObject ) {
    std::string result = jsonObject.dump();
    return ( result == "null" ? "{}" : result );
}

// TODO: rename jsonCastToBool !!!!
// or even remove this one
bool getBool( const Json& value ) {
    return ( !value.empty() && ( ( value.is_boolean() && value.get< bool >() ) ||
                                 ( value.is_number_integer() && value.get< int >() != 0L ) ||
                                 ( value.is_number_float() && value.get< double >() != 0.0 ) ||
                                 ( value.is_string() && !value.empty() && value.get< std::string >() != "0" &&
                                   value.get< std::string >() != "false" ) ) );
}
