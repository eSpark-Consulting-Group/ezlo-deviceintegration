#pragma once

#include <string>
#include <map>
#include "luad/modules/network/network_scan_client.h"

namespace HubPlatform {
namespace LuaNetwork {

class NetworkScanManager final {
public:
    using ClientName = std::string;

    NetworkScanClient& makeScannerClient( const ClientName& clientName );
    void removeScannerClient( const ClientName& clientName );

private:
    std::map< ClientName, std::unique_ptr< NetworkScanClient > > m_scannerClients;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
