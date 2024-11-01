#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class ColorCapability {
    HueSaturationSupported = 0x00,
    EnhancedHueSupported = 0x01,
    ColorLoopSupported = 0x02,
    XyAttributesSupported = 0x03,
    ColorTemperatureSupported = 0x04,

    Unknown = 0xff
};

const char* toString( ColorCapability id );
ColorCapability colorCapabilityFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
