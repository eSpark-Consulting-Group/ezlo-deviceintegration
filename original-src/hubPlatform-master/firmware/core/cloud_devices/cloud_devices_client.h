#pragma once

#include "service_interfaces/script_client.h"
#include "addon_interfaces/addon.h"

class IDbClient;

namespace HubPlatform {
namespace CloudDevices {

class CloudDevicesClient : public ScriptClient {
public:
    CloudDevicesClient( Addons::Addon& parent );

    std::string sendCommand( const std::string& command, const Json& params );

    // NOTE: following code was only used in POC implementaion and can be safely removed
    Json getAllDevicesIds() const;
    Json getDevice( const std::string& deviceId ) const;
    std::string sendCommand( const std::string& commandId,
                             const std::string& deviceId,
                             const std::string& capability,
                             const std::string& command,
                             const Json& params );

private:
    const IDbClient& getDBClient() const;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
