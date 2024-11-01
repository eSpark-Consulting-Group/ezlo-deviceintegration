#pragma once

#include "luad/modules/network/network_scan.h"

#include <tiny/tiny_thread_pool.h>
#include <map>
#include <string>
#include <memory>
#include <functional>

namespace HubPlatform {
namespace LuaNetwork {

class NetworkScanClientError : public std::runtime_error {
public:
    NetworkScanClientError( const std::string& errDesription )
    : runtime_error( "NetworkScanClient: " + errDesription ) {}
};

class NetworkScanClient final {
public:
    static const int simultaneousSearchNumber = 10;
    using AddressVersion = LuaNetwork::AddressVersion;
    using ScanHandle = int;
    using ScannerPtr = std::shared_ptr< INetworkScan >;

    struct Result {
        NetworkScan::OptionalResult scanResult;
        int scanHandle;
        std::string status;
    };
    using ResultCallback = std::function< void( const Result& ) >;

    NetworkScanClient();

    ScanHandle makeScanner( Services::Service& service,
                            AddressVersion format,
                            INetworkScan::Addresses&& addresses,
                            INetworkScan::Ports&& ports,
                            int tries,
                            ResultCallback resultCallback );

    void startScanner( ScanHandle handle );
    void stopScanner( ScanHandle handle );

private:
    ScannerPtr getScanner( ScanHandle handle );
    void removeScanner( ScanHandle handle );

    std::map< ScanHandle, ScannerPtr > m_networkScanners;
    ScanHandle m_handle;
    std::mutex m_mutex;
    tiny::thread_pool m_threadPool;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
