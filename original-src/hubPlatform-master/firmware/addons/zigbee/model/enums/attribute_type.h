#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Todo define other values
enum class AttributeValueType {
    NoData = 0x00,
    BooleanData = 0x10,
    Bitmap8Data = 0x18,
    Bitmap16Data = 0x19,
    Uint8Data = 0x20,
    Uint16Data = 0x21,
    Uint32Data = 0x23,
    Int8Data = 0x28,
    Int16Data = 0x29,
    Enum8Data = 0x30,
    Enum16Data = 0x31,
    StringData = 0x42,
    Eui64Data = 0xF0,
    UnknownData = 0xFF
};

const char* toString( AttributeValueType valueType );
AttributeValueType attributeValueTypeFromString( const std::string& valueType );

}  // namespace Zigbee
}  // namespace HubPlatform
