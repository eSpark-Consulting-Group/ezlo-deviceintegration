#include "policies.h"
#include "resources.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Policies {
using namespace Resources;

void PolicyInfo::setId( std::string id ) {
    m_id = std::move( id );
}

const std::string& PolicyInfo::getId() const {
    return m_id;
}

const std::string& PolicyInfo::getName() const {
    return m_name;
}

bool PolicyInfo::getPersistent() const {
    return m_persistent;
}

const std::string& PolicyInfo::getSecurityConfig() const {
    return m_securityConfig;
}

const std::string& PolicyInfo::getDeviceGroup() const {
    return m_deviceGroup;
}

const JsonArray& PolicyInfo::getMeshbots() const {
    return m_meshbots;
}

Json PolicyInfo::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ "_id" ] = m_id;
    result[ Policy::Name ] = m_name;
    result[ Policy::Persistent ] = m_persistent;
    result[ Policy::ProtectConfig ] = m_securityConfig;
    result[ Policy::DeviceGroup ] = m_deviceGroup;
    result[ Policy::MeshBots ] = m_meshbots;
    return result;
}

bool PolicyInfo::fromJson( const JsonObject& data ) {
    JsonArray meshbots;
    m_id = json_get_str( data, "_id" );
    m_name = json_get_str( data, Policy::Name );
    m_persistent = json_get_bool( data, Policy::Persistent, false );
    m_securityConfig = json_get_str( data, Policy::ProtectConfig );
    m_deviceGroup = json_get_str( data, Policy::DeviceGroup );
    m_meshbots = json_get_arr( data, Policy::MeshBots, meshbots );
    return !m_id.empty() && !m_name.empty();
}

}  // namespace Policies
}  // namespace CoreD
}  // namespace HubPlatform
