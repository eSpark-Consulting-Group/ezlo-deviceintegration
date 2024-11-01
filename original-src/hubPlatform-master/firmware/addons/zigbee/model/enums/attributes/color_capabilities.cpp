#include "zigbee/model/enums/attributes/color_capabilities.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct ColorCapabilityData {
    ColorCapability id;
    const char* name;
};

static const ColorCapabilityData colorCapabilitiesData[] = {
    { ColorCapability::HueSaturationSupported,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorCapabilities::HueSaturationSupported },
    { ColorCapability::EnhancedHueSupported,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorCapabilities::EnhancedHueSupported },
    { ColorCapability::ColorLoopSupported,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorCapabilities::ColorLoopSupported },
    { ColorCapability::XyAttributesSupported,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorCapabilities::XyAttributesSupported },
    { ColorCapability::ColorTemperatureSupported,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorCapabilities::ColorTemperatureSupported },
};

const char* toString( ColorCapability id ) {
    for( const auto& data : colorCapabilitiesData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Color Capability" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

ColorCapability colorCapabilityFromString( const std::string& name ) {
    for( const auto& data : colorCapabilitiesData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Color Capability name" );

    return ColorCapability::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
