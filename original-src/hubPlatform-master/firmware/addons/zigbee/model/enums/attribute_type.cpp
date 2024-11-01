#include "zigbee/model/enums/attribute_type.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

struct AttributeTypeData {
    AttributeValueType typeId;
    const char* name;
};

static const AttributeTypeData attributeTypeData[] = {
    { AttributeValueType::NoData, Resources::Fields::Attribute::ValueType::NoData },
    { AttributeValueType::BooleanData, Resources::Fields::Attribute::ValueType::Bool },
    { AttributeValueType::Bitmap8Data, Resources::Fields::Attribute::ValueType::Bitmap8Data },
    { AttributeValueType::Bitmap16Data, Resources::Fields::Attribute::ValueType::Bitmap16Data },
    { AttributeValueType::Uint8Data, Resources::Fields::Attribute::ValueType::Uint8Data },
    { AttributeValueType::Uint16Data, Resources::Fields::Attribute::ValueType::Uint16Data },
    { AttributeValueType::Uint32Data, Resources::Fields::Attribute::ValueType::Uint32Data },
    { AttributeValueType::Int8Data, Resources::Fields::Attribute::ValueType::Int8Data },
    { AttributeValueType::Int16Data, Resources::Fields::Attribute::ValueType::Int16Data },
    { AttributeValueType::Enum8Data, Resources::Fields::Attribute::ValueType::Enum8Data },
    { AttributeValueType::Enum16Data, Resources::Fields::Attribute::ValueType::Enum16Data },
    { AttributeValueType::StringData, Resources::Fields::Attribute::ValueType::StringData },
    { AttributeValueType::Eui64Data, Resources::Fields::Attribute::ValueType::Eui64Data },
    { AttributeValueType::UnknownData, Resources::Fields::Attribute::ValueType::UnknownData }
};

const char* toString( AttributeValueType valueType ) {
    for( const auto& data : attributeTypeData ) {
        if( data.typeId == valueType ) {
            return data.name;
        }
    }

    return Resources::Fields::Attribute::ValueType::UnknownData;
}

AttributeValueType attributeValueTypeFromString( const std::string& valueType ) {
    for( const auto& data : attributeTypeData ) {
        if( valueType == data.name ) {
            return data.typeId;
        }
    }

    return AttributeValueType::UnknownData;
}

}  // namespace Zigbee
}  // namespace HubPlatform
