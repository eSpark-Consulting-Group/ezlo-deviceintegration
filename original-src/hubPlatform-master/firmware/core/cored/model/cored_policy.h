#pragma once

#include "json_utils/json_fwd.h"
#include "provision_data_utils/provision_data.h"

#include <string>
#include <vector>
#include <set>

namespace HubPlatform {

namespace CoreDRemoteInterface {
class CoreDRemoteInterface;
}

namespace CoreD {
namespace Model {

/* @brief cored's representation of a policy */
class Policy final {
    friend CoreDRemoteInterface::CoreDRemoteInterface;

public:
    Policy( const std::string& name,
            const std::string& securityConfig,
            const std::string& deviceGroup,
            const JsonArray meshbots,
            bool persistent = false );

private:
    Json toJson() const;
    static void checkPolicyAsJson( const Json& device, bool isGetDeviceListCall = false );

private:
    const std::string m_name;           // required
    const std::string m_protectConfig;  // optional
    const std::string m_deviceGroup;    // optional
    JsonArray m_meshbots;               // optional
    bool m_persistent = false;          // required
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
