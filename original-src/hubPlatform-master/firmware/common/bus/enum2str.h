#pragma once

#include <string>
#include <unordered_map>
#include <map>

typedef std::unordered_map< int, std::string > EnumToStrMap;
typedef std::map< std::string, int > StrToEnumMap;

bool enumToStr( const EnumToStrMap& enumToStrMap, const int key, std::string& result, const std::string& unknown );
bool strToEnum( const EnumToStrMap& enumToStrMap, const std::string key, int& result, const int unknown );
std::string enumToStr( const EnumToStrMap& enumToStrMap, const int key, const std::string& unknown );
int strToEnum( const EnumToStrMap& enumToStrMap, const std::string key, const int unknown );
