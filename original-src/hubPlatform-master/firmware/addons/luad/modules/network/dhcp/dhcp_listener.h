#pragma once

#include "json_utils/json_fwd.h"
#include "dhcp.h"
#include "luad/modules/network/network_scan.h"
#include <thread>
#include <memory>

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

class DhcpListener : public INetworkScan {
public:
    using Result = DhcpPacketUnpack::AllHeadersPtr;
    using Status = std::string;
    using ResultCallback = std::function< void( Result&& ) >;
    using FilterCallback = std::function< bool( const Result& ) >;

public:
    explicit DhcpListener( ResultCallback callback );
    ~DhcpListener() override;

    void scan() override;
    void stop() override;
    bool isScanning() const override;

    void setSocket( std::unique_ptr< ISocket >&& socket );
    void setFilter( FilterCallback&& );
    bool filterPacket( const Result& );

private:
    void run();
    void runThrowErrors();
    static Result unpackHeaders( uint8_t* buffer, unsigned length );
    void notify( Result& headersPtr );

private:
    std::atomic_bool m_isRunning;
    ResultCallback m_callback;
    FilterCallback m_filterCallback;
    std::unique_ptr< ISocket > m_socket;
    std::thread m_thread;
};

std::unique_ptr< INetworkScan > makeListener( DhcpListener::ResultCallback&& );

class DhcpSocketError : public std::runtime_error {
public:
    explicit DhcpSocketError( const std::string& err ) noexcept;
};

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform