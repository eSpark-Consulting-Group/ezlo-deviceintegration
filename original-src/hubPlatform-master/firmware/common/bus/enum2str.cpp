#include "enum2str.h"

#include <mutex>

bool enumToStr( const EnumToStrMap& enumToStrMap, const int key, std::string& result, const std::string& unknown ) {
    try {
        result = enumToStrMap.at( key );
        return true;
    }
    catch( ... ) {
        result = unknown;
        return false;
    }
}

bool strToEnum( const EnumToStrMap& enumToStrMap, const std::string key, int& result, const int unknown ) {
    static std::mutex m;
    std::lock_guard< std::mutex > lock( m );
    static std::map< const void*, StrToEnumMap > reverseMaps;
    StrToEnumMap& strToEnumMap = reverseMaps[&enumToStrMap ];
    if( strToEnumMap.empty() )
        for( auto& element : enumToStrMap )
            strToEnumMap[ element.second ] = element.first;
    try {
        result = strToEnumMap.at( key );
        return true;
    }
    catch( ... ) {
        result = unknown;
        return false;
    }
}

std::string enumToStr( const EnumToStrMap& enumToStrMap, const int key, const std::string& unknown ) {
    std::string result;
    enumToStr( enumToStrMap, key, result, unknown );
    return result;
}

int strToEnum( const EnumToStrMap& enumToStrMap, const std::string key, const int unknown ) {
    int result;
    strToEnum( enumToStrMap, key, result, unknown );
    return result;
}
