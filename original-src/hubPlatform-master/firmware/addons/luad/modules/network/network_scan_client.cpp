#include "luad/modules/network/network_scan_client.h"
#include "service_interfaces/service.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace LuaNetwork {

NetworkScanClient::NetworkScanClient()
: m_handle( 0 )
, m_threadPool( simultaneousSearchNumber ) {}

NetworkScanClient::ScanHandle NetworkScanClient::makeScanner( Services::Service& service,
                                                              NetworkScanClient::AddressVersion format,
                                                              INetworkScan::Addresses&& addresses,
                                                              INetworkScan::Ports&& ports,
                                                              int tries,
                                                              ResultCallback resultCallback ) {
    auto handle = ++m_handle;
    NetworkScan::ResultCallback networkScanResultCallback =
        [resultCallback, handle]( const NetworkScan::Status& status, NetworkScan::OptionalResult&& result ) {
            resultCallback( { std::move( result ), handle, status } );
        };
    ScannerPtr scannerPtr = std::make_shared< NetworkScan >(
        service, std::move( addresses ), std::move( ports ), tries, networkScanResultCallback );
    std::lock_guard< std::mutex > lock( m_mutex );
    auto result = m_networkScanners.emplace( std::make_pair( m_handle, std::move( scannerPtr ) ) );
    if( !result.second ) {
        throw NetworkScanClientError{ "fail to register a new NetworkScan" };
    }
    return result.first->first;
}

void NetworkScanClient::stopScanner( NetworkScanClient::ScanHandle handle ) {
    getScanner( handle )->stop();
}

NetworkScanClient::ScannerPtr NetworkScanClient::getScanner( NetworkScanClient::ScanHandle handle ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    auto it = m_networkScanners.find( handle );
    if( it == m_networkScanners.end() ) {
        throw NetworkScanClientError{ "fail to get an NetworkScanClient, handle: " + std::to_string( handle ) };
    }
    return it->second;
}

void NetworkScanClient::startScanner( NetworkScanClient::ScanHandle handle ) {
    m_threadPool.submit_without_future( [this, handle]() {
        auto scanner = getScanner( handle );
        scanner->scan();
        removeScanner( handle );
    } );
}

void NetworkScanClient::removeScanner( NetworkScanClient::ScanHandle handle ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_networkScanners.erase( handle );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
