#pragma once

#include <string>

namespace HubPlatform {

enum class Color {
    Unknown,

    Wwhite,
    Cwhite,
    Red,
    Green,
    Blue,
    Amber,
    Cyan,
    Purple
};

Color colorFromString( const std::string& string );
const std::string& colorToString( Color color );

}  // namespace HubPlatform
