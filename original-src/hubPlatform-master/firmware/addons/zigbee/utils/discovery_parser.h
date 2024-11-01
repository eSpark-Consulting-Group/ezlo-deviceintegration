#pragma once

#include "zigbee/model/enums/attribute_type.h"

#include <stdint-gcc.h>
#include <functional>

namespace HubPlatform {
namespace Zigbee {

class DiscoveryParser {
public:
    DiscoveryParser() = delete;

    static void parseAttributesDiscoveryResponse(
        uint8_t* buffer,
        uint16_t bufLen,
        bool extended,
        std::function< void( uint16_t, AttributeValueType, uint8_t* ) > callback );
};

}  // namespace Zigbee
}  // namespace HubPlatform
