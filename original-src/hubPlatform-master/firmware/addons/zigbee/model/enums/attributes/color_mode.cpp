#include "zigbee/model/enums/attributes/color_mode.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct ColorModeData {
    ColorMode id;
    const char* name;
};

static const ColorModeData colorModeData[] = {
    { ColorMode::ColorHueSaturation,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorMode::ColorHueSaturation },
    { ColorMode::ColorXY, Resources::Fields::Attribute::Formats::ColorControlCluster::ColorMode::ColorXY },
    { ColorMode::ColorTemperature,
      Resources::Fields::Attribute::Formats::ColorControlCluster::ColorMode::ColorTemperature }
};

const char* toString( ColorMode id ) {
    for( const auto& data : colorModeData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Color Mode" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

ColorMode colorModeFromString( const std::string& name ) {
    for( const auto& data : colorModeData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Color Mode name" );

    return ColorMode::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
