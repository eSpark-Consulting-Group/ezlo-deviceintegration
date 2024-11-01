#include "json_approximate_size.h"

size_t jsonObjectSize( const JsonObject& json ) {
    size_t size = 0;
    if( !json.is_object() )
        return size;

    for( auto it = json.begin(); it != json.end(); ++it ) {
        const auto& key = it.key();
        const auto& value = it.value();

        size += key.size() + 3;  // 2 quotes, semicolon
        size += jsonApproximateSize( value );
    }
    size += 2;  // braces
    if( json.size() > 1 )
        size += ( json.size() - 1 );  // commas

    return size;
}

size_t jsonArraySize( const JsonObject& json ) {
    size_t size = 0;
    if( !json.is_array() )
        return size;

    for( auto it = json.begin(); it != json.end(); ++it ) {
        size += jsonApproximateSize( *it );
    }
    size += 2;  // braces
    if( json.size() > 1 )
        size += json.size() - 1;  // commas
    return size;
}

/** Calculates approximate size of a json object's dump without dumping data. */
size_t jsonApproximateSize( const Json& json ) {
    switch( json.type() ) {
    case Json::value_t::boolean:
        return json.get< bool >() ? 4 : 5;  // true, false
    case Json::value_t::string:
        return json.get_ptr< const Json::string_t* >()->size() + 2;  // braces
    case Json::value_t::number_integer:
    case Json::value_t::number_float:
        return json.dump().size();
    case Json::value_t::object:
        return jsonObjectSize( json );
    case Json::value_t::array:
        return jsonArraySize( json );
    default:;
    }

    return json.dump().size();
}
