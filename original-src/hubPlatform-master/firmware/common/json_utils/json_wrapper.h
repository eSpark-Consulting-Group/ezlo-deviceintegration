#pragma once

#include "json_fwd.h"

#include <iostream>
#include <string>

std::string dump( const Json& jsonObject );
bool getBool( const Json& value );

class JsonBuilder final : public Json {
public:
#define JSON( x ) std::move( JsonBuilder() << x ).getJson()
#define JSON_EMPTY_OBJ Json::object()
#define JSON_NULL_VAL nullptr
    JsonBuilder& operator<<( const Json& r ) {
        JsonBuilder& l = ( *this );
        if( savedKey_.empty() ) {
            if( r.type() == Json::value_t::object )
                for( auto it = r.cbegin(); it != r.cend(); ++it )
                    l[ it.key() ] = it.value();
            else
                std::cerr << "ERROR: JsonBuilder(Json): builder has no key or wrong type: " << r << std::endl;
        }
        else {
            l[ savedKey_ ] = r;
            savedKey_.clear();
        }
        return l;
    }
    JsonBuilder& operator<<( const std::string& r ) {
        JsonBuilder& l = ( *this );
        l << r.c_str();
        return l;
    }
    JsonBuilder& operator<<( const char* r ) {
        JsonBuilder& l = ( *this );
        if( savedKey_.empty() ) {
            savedKey_ = r;
        }
        else {
            l[ savedKey_ ] = r;
            savedKey_.clear();
        }
        return l;
    }
    template < typename T >
    JsonBuilder& operator<<( const T& r ) {
        JsonBuilder& l = ( *this );
        if( !savedKey_.empty() ) {
            l[ savedKey_ ] = r;
            savedKey_.clear();
        }
        else
            std::cerr << "ERROR: JsonBuilder(T): builder has no key" << std::endl;
        return l;
    }
    Json& getJson() & {
        savedKey_.clear();
        return ( *this );
    }
    Json&& getJson() && {
        savedKey_.clear();
        return std::move( *this );
    }
    Json& obj() & {
        savedKey_.clear();
        return ( *this );
    }
    Json&& obj() && {
        savedKey_.clear();
        return std::move( *this );
    }

private:
    std::string savedKey_;
};

class JsonArrayBuilder : public Json::array_t {
public:
#define JSON_ARRAY_( ... ) Json::array( { __VA_ARGS__ } )
#define JSON_ARRAY( x ) std::move( JsonArrayBuilder() << x )
#define JSON_EMPTY_ARR Json::array()
    template < typename T >
    JsonArrayBuilder& operator<<( const T& r ) {
        ( *this ).push_back( r );
        return ( *this );
    }
    Json::array_t& arr() {
        return ( *this );
    }
};

class JsonVector : public Json {
public:
    JsonVector()
    : valueKey_( 0 ) {}
    JsonVector& operator<<( const Json& r ) {
        JsonVector& l = ( *this );
        std::string indexStr = std::to_string( valueKey_++ );
        if( !indexStr.empty() )
            l[ indexStr ] = r;
        return l;
    }

private:
    size_t valueKey_;
};
