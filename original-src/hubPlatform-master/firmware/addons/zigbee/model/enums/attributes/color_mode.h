#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class ColorMode {
    ColorHueSaturation = 0x00,
    ColorXY = 0x01,
    ColorTemperature = 0x02,

    Unknown = 0xff
};

const char* toString( ColorMode id );
ColorMode colorModeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
