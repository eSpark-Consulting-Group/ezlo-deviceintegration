#include "zigbee/utils/discovery_parser.h"

#include "log_utils/ezlo_log.h"

extern "C" {
#include <Zigbee/definitions.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
}

namespace HubPlatform {
namespace Zigbee {

void DiscoveryParser::parseAttributesDiscoveryResponse(
    uint8_t* buffer,
    uint16_t bufLen,
    bool extended,
    std::function< void( uint16_t, AttributeValueType, uint8_t* ) > callback ) {
    uint16_t bufIndex = 0;

    while( bufIndex + 3 <= bufLen ) {
        EmberAfAttributeId attributeId;
        uint8_t dataType;
        uint8_t accessControl;
        attributeId = (EmberAfAttributeId) emberAfGetInt16u( buffer, bufIndex, bufLen );
        bufIndex += 2;
        dataType = emberAfGetInt8u( buffer, bufIndex, bufLen );
        bufIndex++;
        if( extended ) {
            accessControl = emberAfGetInt8u( buffer, bufIndex, bufLen );
            bufIndex++;
            callback( attributeId, static_cast< AttributeValueType >( dataType ), &accessControl );
        }
        else {
            callback( attributeId, static_cast< AttributeValueType >( dataType ), nullptr );
        }
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
