#pragma once
#include "luad/modules/network/network_address.h"
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <ifaddrs.h>
#include <string.h>

namespace HubPlatform {
namespace LuaNetwork {

class NetworkInterfacesError : public std::runtime_error {
public:
    NetworkInterfacesError( const std::string& errDescription )
    : runtime_error( "NetworkInterfaces: " + errDescription ) {}
};

class NetworkInterfaces {
public:
    using InterfaceName = std::string;
    using InterfaceNames = std::set< InterfaceName >;
    using Address = std::unique_ptr< IAddress >;
    using Addresses = std::vector< Address >;

    struct Interface {
        InterfaceName name;
        Addresses addresses;
    };
    using Interfaces = std::vector< Interface >;

    NetworkInterfaces();
    ~NetworkInterfaces();

    InterfaceNames getInterfaceNames();

private:
    void getIfaddrs();

    ifaddrs* m_ifaddrs;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
