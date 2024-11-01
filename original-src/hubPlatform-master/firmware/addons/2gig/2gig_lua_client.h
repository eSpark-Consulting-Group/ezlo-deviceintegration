#pragma once

#include "service_interfaces/script_client.h"
#include "addon_interfaces/addon.h"

namespace HubPlatform {
namespace TwoGig {

class LuaClient : public ScriptClient {
public:
    LuaClient( Addons::Addon& addon );

    void reset();

    void startDeviceDiscovering();
    void stopDeviceDiscovering();

    void addDevice( Json&& deviceInfo );
    void removeDevice( const std::vector< uint32_t >& txIds );

    void configureDevice( const std::vector< uint32_t >& txIds, Json&& config );
};

}  // namespace TwoGig
}  // namespace HubPlatform
