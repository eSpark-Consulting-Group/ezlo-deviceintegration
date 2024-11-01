#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <netinet/ip_icmp.h>
#include <cstring>
#include <functional>
#include "common_utils/cpp14.h"
#include "luad/modules/network/socket.h"
#include "luad/modules/network/network_address.h"
#include "luad/modules/network/network_address_container.h"
#include "luad/modules/network/network_ping.h"
#include "luad/modules/network/events_emitter.h"
#include "luad/modules/network/network_interfaces.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace LuaNetwork {

class INetworkScan {
public:
    using Port = int;
    using Ports = std::vector< Port >;
    using Addresses = std::unique_ptr< IAddressContainer >;

    virtual ~INetworkScan() = default;

    virtual void scan() = 0;
    virtual void stop() = 0;
    virtual bool isScanning() const = 0;
};

class NetworkScan : public INetworkScan {
public:
    using Status = std::string;
    struct Result {
        std::string ipAddress;
        std::string macAddress;
        std::string hostName;
        Ports ports;
        unsigned tries;
        unsigned errors;
    };
    using OptionalResult = std::unique_ptr< Result >;
    using ResultCallback = std::function< void( const Status&, OptionalResult&& ) >;
    NetworkScan( Services::Service& service, Addresses&& addresses, Ports&& ports, int tries, ResultCallback callback );
    ~NetworkScan();

    virtual void scan() override;
    virtual void stop() override;
    virtual bool isScanning() const override;

private:
    void scanning();
    std::string getMacAddress( const IAddress& address );
    void sendResult( const IAddress& address, const Ports& ports = Ports(), unsigned tries = 1, unsigned errors = 0 );

    std::atomic_bool m_isRunning;
    Addresses m_addresses;
    Ports m_ports;
    int m_tries{ 1 };
    ResultCallback m_callback;
    Services::Service& m_service;
    std::unique_ptr< ISocket > m_socket;
    std::thread m_thread;
    NetworkInterfaces m_networkInterfaces;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
