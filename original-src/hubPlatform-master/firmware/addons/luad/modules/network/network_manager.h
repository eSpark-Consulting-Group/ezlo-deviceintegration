#pragma once

#include "luad/modules/network/async_sockets_manager.h"
#include "luad/modules/network/network_scan_manager.h"
#include "events_loop/events_loop.h"
#include "service_interfaces/service.h"

#include <string>
#include <memory>
#include <mutex>
#include <limits>

namespace HubPlatform {

namespace UT {
class NetworkManagerTestSuite;
}

namespace LuaNetwork {

using namespace EventsLoop;

class EventsEmitter;

namespace Dhcp {
class Subscriptions;
}

class NetworkManager final {
    friend class UT::NetworkManagerTestSuite;

public:
    using SocketsProcessCall = std::function< void( int connectionHandle ) >;

public:
    NetworkManager( Services::Service& service, std::unique_ptr< EventsEmitter >&& eventsEmitter );
    ~NetworkManager();

    NetworkManager( const NetworkManager& rhs ) = delete;
    NetworkManager& operator=( const NetworkManager& rhs ) = delete;

    int connect( const std::string& groupName, const RemoteAddress& remoteAddress, int options );
    std::size_t send( const std::string& groupName, int connectionHandle, const std::string& data );
    std::size_t sendto( const std::string& groupName,
                        int connectionHandle,
                        const std::string& data,
                        const Host& address,
                        const Port& port );
    std::string receive( const std::string& groupName,
                         int connectionHandle,
                         std::size_t amount = IBuffer::MaxCapacity );
    DataAddressPort receiveFrom( const std::string& groupName,
                                 int connectionHandle,
                                 std::size_t amount = IBuffer::MaxCapacity );
    AddressPort getsockname( const std::string& groupName, int connectionHandle );

    void processOpenedSockets( const std::string& groupName, SocketsProcessCall call );

    void request( const std::string& groupName,
                  int connectionHandle,
                  std::string callback,
                  std::string userData,
                  std::size_t amount );
    void requestFrom( const std::string& groupName,
                      int connectionHandle,
                      std::string callback,
                      std::string userData,
                      std::size_t amount );
    std::string inspectData( const std::string& groupName,
                             int connectionHandle,
                             std::size_t amount = IBuffer::MaxCapacity );
    std::size_t inspectDataSize( const std::string& groupName, int connectionHandle );
    void close( const std::string& groupName, int connectionHandle );

    void setEventsHandler( const std::string& groupName, const std::string& eventHandler );
    void setEventsHandler( const std::string& groupName, const std::string& eventHandler, int connectionHandle );

    NetworkScanClient::ScanHandle startScanning( const std::string& groupName,
                                                 NetworkScanClient::AddressVersion addressFormat,
                                                 INetworkScan::Addresses&& addresses,
                                                 INetworkScan::Ports&& ports,
                                                 const std::string& resultHandler,
                                                 int tries = 1 );

    void stopScanning( const std::string& groupName, NetworkScanClient::ScanHandle handle );

    using DhcpHandle = std::string;
    DhcpHandle startListeningDhcp( const std::string& groupName,
                                   const std::string& resultHandler,
                                   const std::vector< std::string >& operations );
    void stopListeningDhcp( const std::string& groupName, DhcpHandle subscriptionId );

    int createUdp( const std::string& groupName );
    void setPeerName( const std::string& groupName, int connectionHandle, const Host& address, const Port& port );
    void setSockName( const std::string& groupName, int connectionHandle, const Host& address, const Port& port );
    void setOption( const std::string& groupName,
                    int connectionHandle,
                    const std::string& option,
                    const JsonObject& properties );
    void setTimeout( const std::string& groupName, int connectionHandle, std::chrono::milliseconds timeoutMs );

    void reset( const std::string& groupName );

private:
    NetworkManager( Services::Service& service,
                    std::unique_ptr< EventsEmitter >&& eventsEmitter,
                    std::unique_ptr< IEventsLoop > eventsLoop );

private:
    Services::Service& m_service;
    std::unique_ptr< EventsEmitter > m_eventsEmitter;
    std::unique_ptr< IEventsLoop > m_eventsLoop;

    AsyncSocketsManager m_asyncSocketsManager;
    std::mutex m_asyncSocketsManagerMtx;
    NetworkScanManager m_networkScanManager;
    std::mutex m_networkScanManagerMtx;

    std::unique_ptr< Dhcp::Subscriptions > m_subscriptions;
    std::unique_ptr< INetworkScan > m_dhcpListener;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
