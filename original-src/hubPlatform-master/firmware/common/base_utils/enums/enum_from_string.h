#pragma once

#include <string>
#include <map>

namespace HubPlatform {

template < typename Enum, Enum beginValue, Enum endValue, Enum defaultValue >
Enum enumFromString( const std::string& value ) {
    static auto map = []() {
        std::map< std::string, Enum > map;
        for( int i = static_cast< int >( beginValue ); i != static_cast< int >( endValue ); ++i ) {
            map.insert( std::make_pair( toString( static_cast< Enum >( i ) ), static_cast< Enum >( i ) ) );
        }
        return map;
    }();

    auto it = map.find( value );
    if( it != map.end() ) {
        return it->second;
    }

    return defaultValue;
}

template < typename Enum >
Enum enumFromString( const std::string& value ) {
    return enumFromString< Enum, Enum::Begin, Enum::End, Enum::Default >( value );
}

}  // namespace HubPlatform
