#pragma once

#include "networkd/monitoring/nm_interface_traffic_monitor.h"

#include <json_utils/json_fwd.h>

namespace HubPlatform {
namespace Network {

class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;

    virtual const char* getName() const = 0;

    /* active interface it's an interface which is used as an Internet provider for the firmware */
    virtual void onInterfaceActive( bool active ){};

    virtual bool setCustomSettings( Json customSettings ) {
        return false;
    };

    virtual InterfaceTrafficMonitor::TrafficStatistic getTrafficStatistics() const = 0;
};

}  // namespace Network
}  // namespace HubPlatform
