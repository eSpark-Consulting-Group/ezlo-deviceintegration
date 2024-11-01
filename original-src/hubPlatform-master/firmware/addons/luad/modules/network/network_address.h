#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <netinet/ip_icmp.h>

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace LuaNetwork {

class IAddress {
public:
    using AddressUnit = uint32_t;
    using BaseAddress = std::vector< AddressUnit >;
    using Port = int;

    struct SocketAddress {
        sockaddr address;
        uint32_t size;
    };

    IAddress( const BaseAddress& address, Port port );
    IAddress( BaseAddress&& address, Port port ) noexcept;
    virtual ~IAddress() = default;

    virtual const BaseAddress& getAddress() const;
    virtual std::string getAddressString() const = 0;
    virtual IAddress& operator++() = 0;
    virtual bool operator==( const IAddress& other ) const;
    virtual bool operator!=( const IAddress& other ) const;
    virtual int getAddressSize() const = 0;
    virtual SocketAddress getSocketAddress() const = 0;
    std::string getAddressString( char separator, int base ) const;
    Port getPort() const;

    virtual bool setAddress( const BaseAddress& address ) = 0;
    void setPort( Port port );

    friend bool operator<( const IAddress&, const IAddress& );

protected:
    BaseAddress m_address;
    Port m_port;
};

class Ip4Address : public IAddress {
public:
    static const int AddressSize = 4;

    using AddressUnit = uint8_t;
    using Address = std::array< AddressUnit, AddressSize >;

public:
    Ip4Address( const Address& address, Port port );
    Ip4Address( Address&& address, Port port );
    Ip4Address( const Ip4Address& ) = default;
    Ip4Address( Ip4Address&& ) = default;

    virtual IAddress& operator++() override;
    std::string getAddressString() const override;
    virtual int getAddressSize() const override;
    Address getIp4Address() const;
    virtual SocketAddress getSocketAddress() const override;

    virtual bool setAddress( const BaseAddress& address ) override;

    static Address toAddress( const BaseAddress& baseAddress );
};

class Ip6Address : public IAddress {
public:
    static const int AddressSize = 8;
    using AddressUnit = uint16_t;
    using Address = std::array< AddressUnit, AddressSize >;

public:
    Ip6Address( const Address& address, Port port );
    Ip6Address( Address&& address, Port port );
    Ip6Address( const Ip6Address& ) = default;
    Ip6Address( Ip6Address&& ) = default;

    virtual IAddress& operator++() override;
    std::string getAddressString() const override;
    virtual int getAddressSize() const override;
    Address getIp6Address() const;
    virtual SocketAddress getSocketAddress() const override;

    virtual bool setAddress( const BaseAddress& address ) override;

    static Address toAddress( const BaseAddress& baseAddress );
};

bool operator<( const IAddress& address1, const IAddress& address2 );

enum class AddressVersion { IPv4, IPv6 };

std::unique_ptr< IAddress > toIpAddress( AddressVersion version, const std::string& addressStr, IAddress::Port port );

}  // namespace LuaNetwork
}  // namespace HubPlatform
