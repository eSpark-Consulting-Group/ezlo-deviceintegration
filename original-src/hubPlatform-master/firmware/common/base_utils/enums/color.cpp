#include "base_utils/enums/color.h"

#include <map>

namespace HubPlatform {

namespace {

const std::map< Color, std::string > colorToStringMap = {
    { Color::Unknown, "" },    { Color::Wwhite, "wwhite" }, { Color::Cwhite, "cwhite" },
    { Color::Red, "red" },     { Color::Green, "green" },   { Color::Blue, "blue" },
    { Color::Amber, "amber" }, { Color::Cyan, "cyan" },     { Color::Purple, "purple" },
};

const std::map< std::string, Color > stringToColorMap = []() {
    std::map< std::string, Color > map;
    for( const auto& pair : colorToStringMap ) {
        map.emplace( pair.second, pair.first );
    }
    return map;
}();

}  // namespace

Color colorFromString( const std::string& string ) {
    auto it = stringToColorMap.find( string );
    if( it != stringToColorMap.end() ) {
        return it->second;
    }
    return Color::Unknown;
}

const std::string& colorToString( Color color ) {
    auto it = colorToStringMap.find( color );
    if( it != colorToStringMap.end() ) {
        return it->second;
    }
    return colorToStringMap.find( Color::Unknown )->second;
}

}  // namespace HubPlatform
