#include "device_group.h"
#include "resources.h"
#include <utility>

namespace HubPlatform {
namespace CoreD {
namespace EntryDelay {

std::string toString( EntryDelayEnum entryDelay ) {
    switch( entryDelay ) {
    case EntryDelayEnum::normal:
        return "normal";
    case EntryDelayEnum::long_extended:
        return "longExtended";
    case EntryDelayEnum::extended:
        return "extended";
    case EntryDelayEnum::instant:
        return "instant";
    case EntryDelayEnum::none:
        return "none";
    }

    return "none";
}

std::string toString( RoleEnum roleEnum ) {
    switch( roleEnum ) {
    case RoleEnum::user:
        return "user";
    case RoleEnum::house_modes:
        return "house_modes";
    case RoleEnum::sensor_group:
        return "sensor_group";
    }

    return "";
}

EntryDelayEnum fromStringEntryDelay( const std::basic_string< char >& entryDelay ) {
    if( entryDelay == "normal" ) {
        return EntryDelayEnum::normal;
    }
    else if( entryDelay == "longExtended" ) {
        return EntryDelayEnum::long_extended;
    }
    else if( entryDelay == "extended" ) {
        return EntryDelayEnum::extended;
    }
    else if( entryDelay == "instant" ) {
        return EntryDelayEnum::instant;
    }
    else if( entryDelay == "none" ) {
        return EntryDelayEnum::none;
    }
    return EntryDelayEnum::invalid;
}

RoleEnum fromStringRole( const std::basic_string< char >& role ) {
    if( role == "user" ) {
        return RoleEnum::user;
    }
    else if( role == "house_modes" ) {
        return RoleEnum::house_modes;
    }
    else if( role == "sensor_group" ) {
        return RoleEnum::sensor_group;
    }
    return RoleEnum::user;
}

}  // namespace EntryDelay

using namespace Resources;

DeviceGroupInfo::DeviceGroupInfo( std::string id,
                                  std::string name,
                                  JsonArray categories,
                                  JsonArray devices,
                                  JsonArray exceptions,
                                  bool persistent,
                                  EntryDelay::EntryDelayEnum entryDelay,
                                  bool followEntry,
                                  EntryDelay::RoleEnum role,
                                  std::string packageId )
: m_id( std::move( id ) )
, m_name( std::move( name ) )
, m_categories( std::move( categories ) )
, m_devices( std::move( devices ) )
, m_exceptions( std::move( exceptions ) )
, m_persistent( persistent )
, m_entryDelay( entryDelay )
, m_followEntry( followEntry )
, m_role( role )
, m_packageId( std::move( packageId ) ) {}

void DeviceGroupInfo::setId( std::string id ) {
    m_id = std::move( id );
}

const std::string& DeviceGroupInfo::getId() const {
    return m_id;
}

const std::string& DeviceGroupInfo::getName() const {
    return m_name;
}

const JsonArray& DeviceGroupInfo::getCategories() const {
    return m_categories;
}

const JsonArray& DeviceGroupInfo::getDevices() const {
    return m_devices;
}

void DeviceGroupInfo::setDevices( const JsonArray& devices ) {
    m_devices = std::move( devices );
}

const JsonArray& DeviceGroupInfo::getExceptions() const {
    return m_exceptions;
}

void DeviceGroupInfo::setExceptions( const JsonArray& exceptions ) {
    m_exceptions = std::move( exceptions );
}

const std::string DeviceGroupInfo::getRole() const {
    return EntryDelay::toString( m_role );
}

const std::string DeviceGroupInfo::getParentPolicyId() const {
    return m_parentPolicyId;
}

bool DeviceGroupInfo::getPersistent() const {
    return m_persistent;
}

Json DeviceGroupInfo::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ "_id" ] = m_id;
    result[ DeviceGroup::Name ] = m_name;
    result[ DeviceGroup::Categories ] = m_categories;
    result[ DeviceGroup::Devices ] = m_devices;
    result[ DeviceGroup::Exceptions ] = m_exceptions;
    result[ DeviceGroup::Persistent ] = m_persistent;
    result[ DeviceGroup::EntryDelay ] = EntryDelay::toString( m_entryDelay );
    result[ DeviceGroup::FollowEntry ] = m_followEntry;
    result[ DeviceGroup::Role ] = EntryDelay::toString( m_role );
    result[ DeviceGroup::ParentPolicyId ] = m_parentPolicyId;
    result[ DeviceGroup::PackageId ] = m_packageId;

    return result;
}

bool DeviceGroupInfo::fromJson( const JsonObject& data ) {
    std::string status;
    m_id = json_get_str( data, "_id" );
    m_name = json_get_str( data, DeviceGroup::Name );
    JsonArray categories, devices, exceptions;
    json_get_arr( data, DeviceGroup::Categories, categories );
    json_get_arr( data, DeviceGroup::Devices, devices );
    json_get_arr( data, DeviceGroup::Exceptions, exceptions );
    m_persistent = json_get_bool( data, DeviceGroup::Persistent, false );
    m_followEntry = json_get_bool( data, DeviceGroup::FollowEntry, false );
    m_packageId = json_get_str( data, DeviceGroup::PackageId );
    m_entryDelay = EntryDelay::fromStringEntryDelay( json_get_str( data, DeviceGroup::EntryDelay ) );
    m_role = EntryDelay::fromStringRole( json_get_str( data, DeviceGroup::Role ) );
    m_parentPolicyId = json_get_str( data, DeviceGroup::ParentPolicyId );
    m_categories = categories;
    m_devices = devices;
    m_exceptions = exceptions;

    return true;
}

}  // namespace CoreD
}  // namespace HubPlatform
