#pragma once

#include "zigbee/model/enums/attribute_type.h"

#include <stdint-gcc.h>
#include <functional>

namespace HubPlatform {
namespace Zigbee {

class AttributesParser {
public:
    AttributesParser() = delete;

    static void parseAttributesResponse(
        uint8_t* buffer,
        uint16_t bufLen,
        std::function< void( uint16_t, AttributeValueType, std::string, bool ) > callback,
        std::function< void( uint16_t ) > unsupportedAttributesCallback = {} );

    static void parseAttributesReport( uint8_t* buffer,
                                       uint16_t bufLen,
                                       std::function< void( uint16_t, AttributeValueType, std::string ) > callback );

private:
    static void parseAttribute( uint8_t* buffer,
                                uint16_t attributeId,
                                uint16_t& bufIndex,
                                std::function< void( uint16_t, AttributeValueType, std::string ) > callback );
};

}  // namespace Zigbee
}  // namespace HubPlatform
