#include "item_group.h"
#include "resources.h"

namespace HubPlatform {
namespace CoreD {
using namespace Resources;

ItemGroupInfo::ItemGroupInfo( std::string id,
                              std::string name,
                              bool hasGetter,
                              bool hasSetter,
                              bool persistent,
                              JsonArray itemNames,
                              std::string valueType,
                              JsonArray theEnum,
                              std::string valueTypeFamily )
: m_id( std::move( id ) )
, m_name( std::move( name ) )
, m_hasGetter( hasGetter )
, m_hasSetter( hasSetter )
, m_persistent( persistent )
, m_itemNames( std::move( itemNames ) )
, m_valueType( std::move( valueType ) )
, m_enum( std::move( theEnum ) )
, m_valueTypeFamily( std::move( valueTypeFamily ) ) {}

void ItemGroupInfo::setId( std::string id ) {
    m_id = std::move( id );
}

const std::string& ItemGroupInfo::getId() const {
    return m_id;
}

const std::string& ItemGroupInfo::getName() const {
    return m_name;
}

bool ItemGroupInfo::getHasGetter() const {
    return m_hasGetter;
}

bool ItemGroupInfo::getHasSetter() const {
    return m_hasSetter;
}

bool ItemGroupInfo::getPersistent() const {
    return m_persistent;
}

const JsonArray& ItemGroupInfo::getItemNames() const {
    return m_itemNames;
}

CoreD::Model::ValueType ItemGroupInfo::getValueType() const {
    return CoreD::Model::ValueType::fromString( m_valueType );
}

const JsonArray& ItemGroupInfo::getEnum() const {
    return m_enum;
}

const std::string& ItemGroupInfo::getValueTypeFamily() const {
    return m_valueTypeFamily;
}

Json ItemGroupInfo::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ "_id" ] = m_id;
    result[ ItemGroup::Name ] = m_name;
    result[ ItemGroup::HasGetter ] = m_hasGetter;
    result[ ItemGroup::HasSetter ] = m_hasSetter;
    result[ ItemGroup::Persistent ] = m_persistent;
    result[ ItemGroup::ItemNames ] = m_itemNames;
    result[ ItemGroup::ValueType ] = m_valueType;
    result[ ItemGroup::Enum ] = m_enum;
    result[ ItemGroup::ValueTypeFamily ] = m_valueTypeFamily;
    result[ ItemGroup::Role ] = m_role;
    result[ ItemGroup::Info ] = m_info;

    return result;
}

bool ItemGroupInfo::fromJson( const JsonObject& data ) {
    std::string status;
    m_id = json_get_str( data, "_id" );
    m_name = json_get_str( data, ItemGroup::Name );
    m_hasGetter = json_get_bool( data, ItemGroup::HasGetter, false );
    m_hasSetter = json_get_bool( data, ItemGroup::HasSetter, false );
    m_persistent = json_get_bool( data, ItemGroup::Persistent, false );
    JsonArray itemNames, tokenEnum, info;
    json_get_arr( data, ItemGroup::ItemNames, itemNames );
    m_valueType = json_get_str( data, ItemGroup::ValueType );
    json_get_arr( data, ItemGroup::Enum, tokenEnum );
    m_valueTypeFamily = json_get_str( data, ItemGroup::ValueTypeFamily );
    m_role = json_get_str( data, ItemGroup::Role );
    json_get_obj( data, ItemGroup::Info, info );
    m_itemNames = itemNames;
    m_enum = tokenEnum;
    m_info = info;

    return true;
}

const std::string& ItemGroupInfo::getRole() const {
    return m_role;
}

const JsonObject& ItemGroupInfo::getInfoObject() const {
    return m_info;
}

}  // namespace CoreD
}  // namespace HubPlatform
