#include "luad/modules/network/network_scan_manager.h"

namespace HubPlatform {
namespace LuaNetwork {

class NetworkManagerError : public std::runtime_error {
public:
    NetworkManagerError( const std::string& errDesription )
    : runtime_error( "NetworkManager: " + errDesription ) {}
};

NetworkScanClient& NetworkScanManager::makeScannerClient( const NetworkScanManager::ClientName& clientName ) {
    auto it = m_scannerClients.find( clientName );
    if( it != m_scannerClients.end() )
        return *it->second;

    auto result = m_scannerClients.emplace( std::make_pair( clientName, std::make_unique< NetworkScanClient >() ) );
    if( !result.second ) {
        throw NetworkManagerError{ "fail to make a new AsyncSocketsClient" };
    }

    return *result.first->second;
}

void NetworkScanManager::removeScannerClient( const NetworkScanManager::ClientName& clientName ) {
    m_scannerClients.erase( clientName );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
