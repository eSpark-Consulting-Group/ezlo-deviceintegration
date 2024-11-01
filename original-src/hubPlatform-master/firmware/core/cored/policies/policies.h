#pragma once

#include "json_utils/json_fwd.h"
#include "database_client/idbclient.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"

namespace HubPlatform {
namespace CoreD {
namespace Policies {

class PolicyInfo {
public:
    PolicyInfo() = default;

public:
    virtual Json toJson() const;
    virtual bool fromJson( const JsonObject& data );

public:
    void setId( std::string id );
    const std::string& getId() const;
    const std::string& getName() const;
    bool getPersistent() const;
    const std::string& getSecurityConfig() const;
    const std::string& getDeviceGroup() const;
    const JsonArray& getMeshbots() const;

private:
    std::string m_id;
    std::string m_name;
    bool m_persistent{ false };
    std::string m_securityConfig;
    std::string m_deviceGroup;
    JsonArray m_meshbots;
};

}  // namespace Policies
}  // namespace CoreD
}  // namespace HubPlatform
