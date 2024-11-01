#pragma once

#include <string>
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "group.h"

namespace HubPlatform {
namespace CoreD {
namespace EntryDelay {
enum class EntryDelayEnum { normal, long_extended, extended, instant, none, invalid };
enum class RoleEnum { user, house_modes, sensor_group };
std::string toString( EntryDelayEnum entryDelayEnum );
std::string toString( RoleEnum roleEnum );
EntryDelayEnum fromStringEntryDelay( const std::basic_string< char >& entryDelayEnum );
RoleEnum fromStringRole( const std::basic_string< char >& roleEnum );

}  // namespace EntryDelay

class DeviceGroupInfo : public GroupInfo {
public:
    DeviceGroupInfo() = default;
    DeviceGroupInfo( std::string id,
                     std::string name,
                     JsonArray categories,
                     JsonArray devices,
                     JsonArray exceptions,
                     bool persistent,
                     EntryDelay::EntryDelayEnum entryDelay,
                     bool followEntry,
                     EntryDelay::RoleEnum role,
                     std::string packageId );

public:
    void setId( std::string id );
    const std::string& getId() const;
    const std::string& getName() const;
    const JsonArray& getCategories() const;
    const JsonArray& getDevices() const;
    void setDevices( const JsonArray& devices );
    const JsonArray& getExceptions() const;
    void setExceptions( const JsonArray& exceptions );
    const std::string getRole() const;
    const std::string getParentPolicyId() const;
    bool getPersistent() const;

public:
    Json toJson() const override;
    bool fromJson( const JsonObject& data ) override;

private:
    std::string m_id;
    std::string m_name;
    JsonArray m_categories;
    JsonArray m_devices;
    JsonArray m_exceptions;
    bool m_persistent{ false };
    EntryDelay::EntryDelayEnum m_entryDelay;
    bool m_followEntry{ false };
    EntryDelay::RoleEnum m_role;
    std::string m_parentPolicyId;
    std::string m_packageId;
};

}  // namespace CoreD
}  // namespace HubPlatform
