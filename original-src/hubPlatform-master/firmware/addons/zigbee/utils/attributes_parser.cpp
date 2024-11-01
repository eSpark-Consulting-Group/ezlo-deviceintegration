#include "zigbee/utils/attributes_parser.h"
#include "zigbee/utils/attribute_string_normalizer.h"

#include "log_utils/ezlo_log.h"

#include <functional>

extern "C" {
#include <Zigbee/definitions.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
}

namespace HubPlatform {
namespace Zigbee {

void AttributesParser::parseAttributesResponse(
    uint8_t* buffer,
    uint16_t bufLen,
    std::function< void( uint16_t, AttributeValueType, std::string, bool ) > callback,
    std::function< void( uint16_t ) > unsupportedAttributesCallback ) {
    // Each record in the response has a two-byte attribute id and a one-byte
    // status.  If the status is SUCCESS, there will also be a one-byte type and
    // variable-length data.
    uint16_t bufIndex = 0;
    while( bufIndex + 3 <= bufLen ) {
        EmberAfAttributeId attributeId = (EmberAfAttributeId) emberAfGetInt16u( buffer, bufIndex, bufLen );
        bufIndex += 2;
        EmberAfStatus status = (EmberAfStatus) emberAfGetInt8u( buffer, bufIndex, bufLen );
        bufIndex++;

        if( EMBER_ZCL_STATUS_SUCCESS == status ) {
            parseAttribute(
                buffer,
                attributeId,
                bufIndex,
                std::bind( callback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, true ) );
        }
        else {
            Log::print( Log::PriorityLevel::DEBUG,
                        "Problem with attribute parse, attributeId: 0x%04x, status: 0x%02x",
                        attributeId,
                        status );

            if( status == EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE ) {
                unsupportedAttributesCallback( attributeId );
            }
            else {
                callback( attributeId, AttributeValueType::UnknownData, "", false );
            }
        }
    }
}

void AttributesParser::parseAttributesReport(
    uint8_t* buffer,
    uint16_t bufLen,
    std::function< void( uint16_t, AttributeValueType, std::string ) > callback ) {
    // Each record in the response has a two-byte attribute id and a one-byte
    // status.  If the status is SUCCESS, there will also be a one-byte type and
    // variable-length data.
    uint16_t bufIndex = 0;
    while( bufIndex + 3 <= bufLen ) {
        EmberAfAttributeId attributeId = (EmberAfAttributeId) emberAfGetInt16u( buffer, bufIndex, bufLen );
        bufIndex += 2;

        parseAttribute( buffer, attributeId, bufIndex, callback );
    }
}

void AttributesParser::parseAttribute( uint8_t* buffer,
                                       uint16_t attributeId,
                                       uint16_t& bufIndex,
                                       std::function< void( uint16_t, AttributeValueType, std::string ) > callback ) {
    uint8_t dataType, dataSize;
    dataType = emberAfGetInt8u( buffer, bufIndex, bufLen );
    bufIndex++;

    // For strings, the data size is the length of the string (specified by
    // the first byte of data) plus one for the length byte itself.  For
    // everything else, the size is just the size of the data type.
    dataSize = ( emberAfIsThisDataTypeAStringType( dataType ) ? emberAfStringLength( buffer + bufIndex ) + 1 :
                                                                emberAfGetDataSize( dataType ) );
    std::string value;
    auto dataTypeEnum = static_cast< AttributeValueType >( dataType );
    switch( dataTypeEnum ) {
    case AttributeValueType::NoData:
        value = "";
        break;
    case AttributeValueType::BooleanData:
        value = ( *static_cast< uint8_t* >( &buffer[ bufIndex ] ) ) ? "true" : "false";
        break;
    case AttributeValueType::Enum8Data:
    case AttributeValueType::Bitmap8Data:
    case AttributeValueType::Uint8Data: {
        value = std::to_string( *static_cast< uint8_t* >( &buffer[ bufIndex ] ) );
    } break;
    case AttributeValueType::Enum16Data:
    case AttributeValueType::Uint16Data:
    case AttributeValueType::Bitmap16Data: {
        value = std::to_string( *reinterpret_cast< uint16_t* >( &buffer[ bufIndex ] ) );
    } break;
    case AttributeValueType::Uint32Data: {
        value = std::to_string( *reinterpret_cast< uint32_t* >( &buffer[ bufIndex ] ) );
    } break;
    case AttributeValueType::Int8Data: {
        value = std::to_string( *reinterpret_cast< int8_t* >( &buffer[ bufIndex ] ) );
    } break;
    case AttributeValueType::Int16Data: {
        value = std::to_string( *reinterpret_cast< int16_t* >( &buffer[ bufIndex ] ) );
    } break;
    case AttributeValueType::StringData:
        value = std::string( (char*) buffer + bufIndex, dataSize );
        AttributeStringNormalizer::normalize( value );
        break;
    case AttributeValueType::Eui64Data: {
        char valueBuffer[ 17 ];
        uint8_t* pEui64 = &buffer[ bufIndex ];
        snprintf( valueBuffer,
                  sizeof( valueBuffer ),
                  "%02x%02x%02x%02x%02x%02x%02x%02x",
                  pEui64[ 7 ],
                  pEui64[ 6 ],
                  pEui64[ 5 ],
                  pEui64[ 4 ],
                  pEui64[ 3 ],
                  pEui64[ 2 ],
                  pEui64[ 1 ],
                  pEui64[ 0 ] );
        value = std::string( valueBuffer, sizeof( valueBuffer ) );
        AttributeStringNormalizer::normalize( value );
    } break;
    default:
        value = "unknown";
        break;
    }
    bufIndex += dataSize;

    callback( attributeId, dataTypeEnum, value );
}

}  // namespace Zigbee
}  // namespace HubPlatform
