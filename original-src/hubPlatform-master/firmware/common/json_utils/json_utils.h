#pragma once

#include "json_wrapper.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>
#include <typeinfo>
#include <functional>
#include <stdexcept>
#include <map>

class JsonError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class WrongParamsError : public JsonError {
public:
    explicit WrongParamsError( const std::string& key )
    : JsonError{ key } {}
};

class WrongFieldError : public JsonError {
public:
    explicit WrongFieldError( const std::string& key )
    : JsonError{ key } {}
};

static inline std::string json_write( const Json& vals ) {
    return vals.dump();
}

static inline bool json_get_obj( const JsonObject& vals, const std::string& key, JsonObject& res ) {
    if( !res.is_object() )
        res = JSON_EMPTY_OBJ;
    if( !vals.count( key ) )
        return false;
    const auto& obj = vals.at( key );
    if( !obj.is_object() )
        return false;
    res = obj;
    return true;
}

/* FIXME: basic_json::at throw an exception if a json value isn't an object... */
static inline bool json_get_arr( const JsonObject& vals, const std::string& key, JsonArray& res ) {
    if( !res.is_array() )
        res = JSON_EMPTY_ARR;
    if( !vals.count( key ) )
        return false;
    const auto& arr = vals.at( key );
    if( !arr.is_array() )
        return false;
    res = arr;
    return true;
}

template < typename T >
static inline bool json_get_element( const JsonArray& vals, std::size_t index, T& res ) {
    if( !vals.is_array() || index >= vals.size() )
        return false;
    res = vals[ index ];
    return true;
}

static inline bool getJsonArray( const std::string& key, const JsonObject& vals, JsonArray& res ) {
    return json_get_arr( vals, key, res );
}

template < typename T >
static inline bool json_get( const Json& vals, T& res ) {
    try {
        res = vals.get< T >();
        return true;
    }
    catch( ... ) {
        return false;
    }
}

template < typename T >
static inline bool json_get( const Json& vals, const char* key, T& res ) noexcept {
    try {
        const auto it = vals.find( key );
        if( it == vals.end() )
            return false;

        if( it->is_boolean() )
            res = it->get< bool >();
        else
            res = it->get< T >();

        return true;
    }
    catch( ... ) {
        return false;
    }
}

static inline bool json_get( const Json& vals,
                             const std::vector< Json >& keyPath,
                             std::function< void( const Json& result ) > callback ) noexcept {
    try {
        const Json* valsPtr = &vals;
        for( const auto& key : keyPath ) {
            if( key.is_number_integer() ) {
                std::size_t index = key;
                if( !valsPtr->is_array() || index >= valsPtr->size() )
                    return false;
                valsPtr = &( *valsPtr )[ index ];
            }
            else if( key.is_string() ) {
                const auto it = valsPtr->find( key );
                if( it == valsPtr->end() ) {
                    return false;
                }
                valsPtr = &*it;
            }
            else {
                return false;
            }
        }
        callback( *valsPtr );
        return true;
    }
    catch( ... ) {
        if( Log::getLogLevel() >= Log::PriorityLevel::TRACE ) {
            std::string path;
            for( const auto& key : keyPath ) {
                path += key.dump() + ".";
            }
        }
        return false;
    }
}

template < typename T >
static inline bool json_get( const Json& vals, const std::vector< Json >& keyPath, T& result ) noexcept {
    return json_get( vals, keyPath, [&result]( const Json& resultJson ) { result = resultJson.get< T >(); } );
}

static inline bool json_get( const Json& vals, const std::vector< Json >& keyPath, Json& result ) noexcept {
    return json_get( vals, keyPath, [&result]( const Json& resultJson ) { result = resultJson; } );
}

static inline bool json_get( const Json& vals, const char* key, Json& res ) {
    auto it = vals.find( key );
    if( it == vals.cend() )
        return false;
    res = *it;
    return true;
}

template < typename T >
static inline bool json_get( const Json& vals, const std::string& key, T& res ) {
    return json_get( vals, key.c_str(), res );
}

static inline bool json_get( const Json& vals, const char* key, bool& res ) {
    // TODO can remove try/catch once server is stable
    try {
        const auto it = vals.find( key );
        if( it == vals.end() )
            return false;

        res = it->get< bool >();
        return true;
    }
    catch( ... ) {
        res = false;
        return false;
    }
}

static inline std::string json_get_str( const Json& obj, const std::string& key ) {
    std::string ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return std::string();
}

static inline std::string json_get_str( const Json& obj ) {
    std::string ret;
    if( json_get( obj, ret ) )
        return ret;
    return std::string();
}

template < typename T >
static inline T json_get_number( const Json& obj, const std::string& key, T default_val = 0 ) {
    T ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline unsigned int json_get_uint( const Json& obj, const std::string& key, unsigned int default_val = 0 ) {
    unsigned int ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline unsigned int json_get_uint( const Json& obj, unsigned int default_val = 0 ) {
    unsigned int ret;
    if( json_get( obj, ret ) )
        return ret;
    return default_val;
}

static inline int json_get_int( const Json& obj, const std::string& key, int default_val = -1 ) {
    int ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline int64_t json_get_int64( const Json& obj, const std::string& key, int64_t default_val = -1 ) {
    int64_t ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline int64_t json_get_uint64( const Json& obj, const std::string& key, uint64_t default_val = 0 ) {
    uint64_t ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline bool json_get_bool( const Json& obj, const std::string& key, bool default_val = false ) {
    bool ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline float json_get_float( const Json& obj, const std::string& key, float default_val = 0 ) {
    float ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

template < typename T >
static inline std::vector< T > json_get_vec( const JsonArray& jsonArray ) {
    std::vector< T > result;
    result.reserve( jsonArray.size() );

    T val;
    for( const auto& element : jsonArray ) {
        if( json_get( element, val ) ) {
            result.push_back( std::move( val ) );
        }
    }

    return result;
}

template < typename T >
static inline std::vector< T > json_get_vec( const Json& obj, const std::string& key ) {
    std::vector< T > result;

    JsonArray jsonArray = JSON_EMPTY_ARR;
    if( json_get_arr( obj, key, jsonArray ) ) {
        result = json_get_vec< T >( jsonArray );
    }

    return result;
}

static inline double json_get_double2( const Json& obj, const std::string& key, double default_val = 0.0 ) {
    double ret;
    if( json_get( obj, key, ret ) )
        return ret;
    return default_val;
}

static inline void json_remove( Json& obj, const std::string& key ) {
    try {
        obj.erase( key );
    }
    catch( ... ) {
        Log::error( cc::info( "json_remove()" ) + cc::error( " got exception while removing key " ) + cc::warn( key ) +
                    cc::error( " from " ) + cc::j( obj ) );
    }
}

static inline void json_add_bool( Json& vals, const std::string& key, bool v ) {
    vals[ key ] = v;
}

static inline void json_add_int( Json& vals, const std::string& key, int v ) {
    vals[ key ] = v;
}

static inline void json_add_uint( Json& vals, const std::string& key, unsigned v ) {
    vals[ key ] = v;
}

static inline void json_add_long( Json& vals, const std::string& key, long v ) {
    vals[ key ] = v;
}

static inline void json_add_int64( Json& vals, const std::string& key, int64_t v ) {
    vals[ key ] = v;
}

static inline void json_add_float( Json& vals, const std::string& key, float v ) {
    vals[ key ] = v;
}

static inline void json_add_double( Json& vals, const std::string& key, double v ) {
    vals[ key ] = v;
}

static inline void json_add_str( Json& vals, const std::string& key, const std::string& v ) {
    vals[ key ] = v;
}

static inline void json_add_null( Json& vals, const std::string& key ) {
    vals[ key ] = nullptr;
}

static inline void json_add_str_array( Json& vals, const std::string& key, const std::vector< std::string >& v ) {
    vals[ key ] = v;
}

static inline bool parseJson( const std::string& jsonStr, Json& result ) {
    if( jsonStr.empty() )
        return false;
    try {
        result = Json::parse( jsonStr );
    }
    catch( ... ) {
        return false;
    }
    return true;
}

static inline Json parseJson( const std::string& jsonStr ) {
    if( jsonStr.empty() )
        return JSON_EMPTY_OBJ;
    try {
        return Json::parse( jsonStr );
    }
    catch( ... ) {
    }
    return JSON_EMPTY_OBJ;
}

static inline void jsonSort( Json& array, const Json& ids, const std::string& field = "_id" ) {
    if( array.size() < 2 || ids.empty() )
        return;
    //	std::map<std::string, const Json &> idsMap; // l_sergiy: incompatible with gcc 7.2 ARM and C++ 17
    std::map< std::string, Json > idsMap;
    for( const Json& value : array )
        if( value.is_object() ) {
            std::string id;
            if( json_get( value, field, id ) ) {
                idsMap.insert( { id, value } );
            }
        }
    Json result = JSON_EMPTY_ARR;
    for( const std::string& id : ids )
        if( idsMap.count( id ) ) {
            result.push_back( idsMap.at( id ) );
            idsMap.erase( id );
        }
    for( const auto& element : idsMap )
        result.push_back( element.second );
    array = result;
}

static inline void jsonCopy( const Json& from, const std::string& field, Json& to ) {
    try {
        to[ field ] = from.at( field );
    }
    catch( ... ) {
        Log::error( cc::info( "jsonCopy()" ) + cc::error( " failed to copy field " ) + cc::warn( field ) +
                    cc::error( " from JSON " ) + cc::j( from ) + cc::error( " to JSON " ) + cc::j( to ) );
    }
}

static inline void jsonCopy( const Json& from, const std::vector< std::string >& fields, Json& to ) {
    for( const auto& field : fields )
        jsonCopy( from, field, to );
}

static inline void jsonCopy( const Json& from, const std::string& field, Json& to, const std::string& toField ) {
    try {
        to[ toField ] = from.at( field );
    }
    catch( ... ) {
        Log::error( cc::info( "jsonCopy( fromField=" ) + cc::warn( field ) + cc::info( ", toField=" ) +
                    cc::warn( toField ) + cc::info( ")" ) + cc::error( " failed, from JSON " ) + cc::j( from ) +
                    cc::error( " to JSON " ) + cc::j( to ) );
    }
}

static inline const JsonObject& jsonGetObject( const JsonObject& jsonObject, const std::string& field ) {
    try {
        const JsonObject& arguments = jsonObject.at( field );
        return arguments;
    }
    catch( ... ) {
        Log::error( cc::info( "jsonGetObject()" ) + cc::error( " failed to get field " ) + cc::warn( field ) +
                    cc::error( " from JSON " ) + cc::j( jsonObject ) );
        static const JsonObject emptyJson = JSON_EMPTY_OBJ;
        return emptyJson;
    }
}

static inline JsonObject json_get_arr_obj( const JsonObject& joSrc,       // must be object
                                           const std::string& arrayName,  // prop name, expect it will have array value
                                           const std::string& field,
                                           const std::string& value ) {
    if( joSrc.empty() )
        return JSON_EMPTY_OBJ;
    if( !joSrc.is_object() )
        return JSON_EMPTY_OBJ;
    JsonObject jarrPropValue = JSON_EMPTY_ARR;
    if( !json_get_arr( joSrc, arrayName, jarrPropValue ) )
        return JSON_EMPTY_OBJ;
    if( !jarrPropValue.is_array() )
        return JSON_EMPTY_OBJ;
    for( JsonObject& joElement : jarrPropValue ) {
        if( joElement.is_object() ) {
            if( joElement.count( field ) > 0 && json_get_str( joElement, field ) == value )
                return joElement;
        }
    }
    return JSON_EMPTY_OBJ;
}

/* @brief check whether a pair (key, bool) exists
 * @note if a pair doesn't exist or a value isn't bool throws an exception */
static inline void jsonCheckBool( const Json& obj, const std::string& key, bool expectField = true ) {
    bool ret;
    if( expectField && json_get( obj, key, ret ) != true ) {
        throw std::runtime_error{ std::string{ "fail to fetch a boolean at such key: " } + "\"" + key + "\"" };
    }
    else if( !expectField && json_get( obj, key, ret ) == true ) {
        throw std::runtime_error{ std::string{ "there is unexpected key: " } + "\"" + key + "\"" };
    }
}

/* @brief check whether a pair (key, int) exists
 * @note if a pair doesn't exist or a value isn't int throws an exception */
static inline void jsonCheckInt( const Json& obj, const std::string& key ) {
    int ret;
    if( json_get( obj, key, ret ) != true ) {
        throw std::runtime_error{ std::string{ "fail to fetch an integer at such key: " } + "\"" + key + "\"" };
    }
}

static inline void jsonCheckFloat( const Json& obj, const std::string& key ) {
    float ret;
    if( json_get( obj, key, ret ) != true ) {
        throw std::runtime_error{ std::string{ "fail to fetch an float at such key: " } + "\"" + key + "\"" };
    }
}

struct JsonStringChecks final {
    enum Enum { NO_CHECKS = 1 << 0, RESTRICTED_LENGTH = 1 << 1, NON_EMPTY = 1 << 2 };

    enum ELimits {
        MAX_LENGTH_LIMIT = 128,
    };
};

static inline JsonStringChecks::Enum operator|( JsonStringChecks::Enum a, JsonStringChecks::Enum b ) {
    return static_cast< JsonStringChecks::Enum >( static_cast< int >( a ) | static_cast< int >( b ) );
}

static inline JsonStringChecks::Enum operator&( JsonStringChecks::Enum a, JsonStringChecks::Enum b ) {
    return static_cast< JsonStringChecks::Enum >( static_cast< int >( a ) & static_cast< int >( b ) );
}

/* @brief check whether a key exists */
static inline bool json_contains( const Json& obj, const std::string& key ) {
    return obj.is_object() && obj.find( key ) != obj.cend();
}

static inline void jsonCheckString( const std::string& string,
                                    JsonStringChecks::Enum checks = JsonStringChecks::RESTRICTED_LENGTH |
                                                                    JsonStringChecks::NON_EMPTY,
                                    int lengthLimit = JsonStringChecks::MAX_LENGTH_LIMIT ) {
    if( checks & JsonStringChecks::RESTRICTED_LENGTH ) {
        if( string.size() > lengthLimit ) {
            throw std::runtime_error{ "exceeded the max allowed length" };
        }
    }

    if( checks & JsonStringChecks::NON_EMPTY ) {
        if( string.empty() ) {
            throw std::runtime_error{ "is empty" };
        }
    }
}

/* @brief check whether a pair (key, string) exists and apply some string checks if the pair exists
 * @note if a pair doesn't exist or a value isn't string or checks failed throws an exception */
static inline void jsonCheckString( const Json& obj,
                                    const std::string& key,
                                    JsonStringChecks::Enum checks = JsonStringChecks::RESTRICTED_LENGTH |
                                                                    JsonStringChecks::NON_EMPTY,
                                    int lengthLimit = JsonStringChecks::MAX_LENGTH_LIMIT ) {
    std::string string;

    if( json_get( obj, key, string ) != true )
        throw std::runtime_error{ std::string{ "fail to fetch a string at such key: " } + "\"" + key + "\"" };

    try {
        return jsonCheckString( string, checks, lengthLimit );
    }
    catch( ... ) {
        throw std::runtime_error{ std::string{ "fail to check a string at such key: " } + "\"" + key + "\"" };
    }
}

/* @brief check whether a pair (key, string) exists with an additional required parameter check */
static inline void jsonCheckString( const JsonObject& params, const std::string& key, bool required ) {
    if( params.find( key ) != params.cend() ) {
        if( !params[ key ].is_string() ) {
            throw WrongParamsError{ key };
        }
    }
    else {
        if( required ) {
            throw WrongFieldError{ key };
        }
    }
}

/* @brief check whether a pair (key, JSON_OBJECT) exists
 * @note if a pair doesn't exist throws an exception */
static inline void jsonCheckObject( const Json& obj, const std::string& key ) {
    Json jsObj;
    if( !json_get( obj, key, jsObj ) || !jsObj.is_object() ) {
        throw std::runtime_error{ std::string{ "fail to fetch an object at such key: " } + "\"" + key + "\"" };
    }
}

/* @brief check whether a pair (key, JSON_OBJECT) exists with an additional required parameter check */
static inline void jsonCheckObject( const JsonObject& params, const std::string& key, bool required ) {
    if( params.find( key ) != params.cend() ) {
        if( !params[ key ].is_object() ) {
            throw WrongParamsError{ key };
        }
    }
    else {
        if( required ) {
            throw WrongFieldError{ key };
        }
    }
}

/* @brief check whether a pair (key, JSON_ARRAY) exists
 * @note if a pair doesn't exist throws an exception */
static inline void jsonCheckArray( const Json& obj, const std::string& key ) {
    Json jsArray;
    if( !json_get( obj, key, jsArray ) || !jsArray.is_array() ) {
        throw std::runtime_error{ std::string{ "fail to fetch an array at such key: " } + "\"" + key + "\"" };
    }
}

/* @brief apply a @c fn function object to an json array, if exists, at @c key into @c obj json object
 * @note if there's no pair with such key or it's not an array throws an exception
 * @note @c fn function object may throw exceptions */
template < typename Fn >
static inline void jsonCheckArray( const Json& obj, const std::string& key, Fn&& fn ) {
    Json jsArray;

    jsonCheckArray( obj, key );

    try {
        json_get( obj, key, jsArray );
        fn( const_cast< const Json& >( jsArray ) );
    }
    catch( const std::exception& exception ) {
        throw std::runtime_error{ "array at \"" + key + "\" key: " + exception.what() };
    }
    catch( ... ) {
        throw std::runtime_error{ std::string{ "fail to apply a function object for an array at " } + "\"" + key +
                                  "\" key" };
    }
}

/** @brief Fetches array of string out of a json array.
 *  @note Throws if one of the elements is not a string. */
static inline void fromJsonStringArray( const JsonArray& arr, std::function< void( const std::string& ) > inserter ) {
    for( const std::string& element : arr )
        inserter( element );
}

/* @brief check whether a pair (key, JSON_ARRAY) exists with an additional required parameter check
 * @note if a pair doesn't exist and contents of array is not a string throws an exception */
static inline void jsonCheckStringArray( const JsonObject& params, const std::string& key, bool required ) {
    if( params.find( key ) != params.cend() ) {
        if( !params[ key ].is_array() ) {
            throw WrongParamsError{ key };
        }
        auto& strings = params[ key ];
        for( auto& element : strings ) {
            if( !element.is_string() ) {
                throw WrongParamsError{ key };
            }
        }
    }
    else {
        if( required ) {
            throw WrongFieldError{ key };
        }
    }
}
static inline Json jsonDiff( const Json& prev, const Json& next, bool flat = false ) {
    Json difference;
    if( !prev.is_object() || !next.is_object() ) {
        return next;
    }

    for( auto it = next.begin(); it != next.end(); ++it ) {
        const auto itFind = prev.find( it.key() );
        if( itFind == prev.end() || itFind->type() != it->type() ) {
            difference[ it.key() ] = it.value();
        }
        else {
            bool equal;
            if( itFind->is_number_float() ) {
                auto prevValue = itFind->get< float >();
                auto nextValue = it->get< float >();
                equal = std::abs( nextValue - prevValue ) <= std::numeric_limits< float >::epsilon();
            }
            else {
                equal = itFind.value() == it.value();
            }
            if( !equal ) {
                auto valueDiff = flat ? it.value() : jsonDiff( itFind.value(), it.value() );
                if( valueDiff.is_array() || !valueDiff.empty() ) {
                    difference[ it.key() ] = valueDiff;
                }
            }
        }
    }
    for( auto it = prev.begin(); it != prev.end(); ++it ) {
        if( next.find( it.key() ) == next.end() ) {
            difference[ it.key() ] = {};
        }
    }
    return difference;
}

static inline JsonObject json_object_keys( const JsonObject& obj ) {
    JsonObject result = JSON_EMPTY_ARR;
    if( obj.is_object() ) {
        for( auto it = obj.begin(); obj.end() != it; ++it ) {
            result.push_back( it.key() );
        }
    }
    return result;
}

/** Fetch std::map<string, Value> from json object by key.
 * For an ancient nlohmann_json version, modern ones have this built-in. */
template < typename Value >
static inline std::map< std::string, Value > json_get_map( const Json& obj, const std::string& key, Value* = nullptr ) {
    std::map< std::string, Value > result;

    if( !obj.count( key ) )
        return result;

    auto& from = obj.at( key );
    try {
        if( from.is_object() ) {
            for( auto it = from.begin(); it != from.end(); ++it ) {
                result.emplace( it.key(), it.value().get< Value >() );
            }
        }
    }
    catch( const std::exception& e ) {
        Log::error( cc::info( "json_get_map()" ) + cc::error( " failed to fetch map for key " ) + cc::warn( key ) +
                    cc::error( " from JSON " ) + cc::j( from ) );
    }
    return result;
}

/** Fetch std::multimap<string, Value> from json array of objects with (key, value) fields. */
template < typename Value >
static inline std::multimap< std::string, Value > json_get_multimap( const JsonArray& from,
                                                                     const std::string& keyField,
                                                                     const std::string& valueField,
                                                                     Value* = nullptr ) {
    std::multimap< std::string, Value > result;

    if( keyField.empty() || valueField.empty() )
        return result;

    try {
        if( from.is_array() ) {
            for( auto& element : from ) {
                if( element.is_object() ) {
                    auto keyIt = element.find( keyField );
                    auto valueIt = element.find( valueField );

                    if( keyIt != element.end() && keyIt->is_string() && valueIt != element.end() ) {
                        result.emplace( keyIt->get< std::string >(), valueIt->get< Value >() );
                    }
                }
            }
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "failed to fetch multimap form JSON " ) + cc::j( from ) + " for (" + keyField + ", " +
                      valueField + ")" );
    }
    return result;
}

/* check if an entry exists in a JSON array */
static inline bool json_array_is_int_present( const JsonArray& array, int toFind ) {
    bool present = false;
    for( const auto& entry : array ) {
        if( entry == toFind ) {
            present = true;
            break;
        }
    }
    return present;
}

static inline bool json_array_is_string_present( const JsonArray& array, const std::string& toFind ) {
    bool present = false;
    for( const auto& entry : array ) {
        if( entry == toFind ) {
            present = true;
            break;
        }
    }
    return present;
}
