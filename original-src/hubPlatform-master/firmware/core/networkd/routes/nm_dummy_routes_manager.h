#pragma once

#include "networkd/routes/nm_routes_manager.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/utils/nm_utils.h"

#include "system_utils/system.h"

namespace HubPlatform {
namespace Network {

class DummyRoutesManager : public RoutesManager {
public:
    std::string checkAndFixRoutes( const std::vector< Json >& interfaces ) override {
        return getDefaultRouteDevice().empty() ? "" : Resources::EthernetDeviceName;
    };

    std::string getDefaultRouteDevice() override {
        return System::getAwkOutput( "ip route | awk '$1==\"default\" && (NF<=5 || $6==\"proto\") {print $5}'" );
    }
};

}  // namespace Network
}  // namespace HubPlatform
