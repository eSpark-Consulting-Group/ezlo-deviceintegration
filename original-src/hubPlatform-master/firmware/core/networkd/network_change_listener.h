#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

class NetworkChangeListener {
public:
    struct NetworkInfo {
        std::string ipv4;
        std::string type;
        std::string interfaceId;
    };

    virtual ~NetworkChangeListener() = default;

    virtual void onNetworkInfoChanged( Json diff ) = 0;
    virtual void onInternetAvailabilityChanged( bool internetAvailable ) = 0;
    virtual void onInterfaceStatusChanged( const std::string& interfaceId,
                                           const std::string& interfaceType,
                                           bool isConnected ) = 0;
    virtual void onCurrentNetworkChanged( const NetworkInfo& newIpAddress ) = 0;
};

}  // namespace Network
}  // namespace HubPlatform
