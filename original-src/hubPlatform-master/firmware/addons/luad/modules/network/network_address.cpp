#include "luad/modules/network/network_address.h"
#include <sstream>

namespace HubPlatform {
namespace LuaNetwork {

IAddress::IAddress( const IAddress::BaseAddress& address, IAddress::Port port )
: m_address( address )
, m_port( port ) {}

IAddress::IAddress( IAddress::BaseAddress&& address, IAddress::Port port ) noexcept
: m_address( std::move( address ) )
, m_port( port ) {}

const IAddress::BaseAddress& IAddress::getAddress() const {
    return m_address;
}

std::string IAddress::getAddressString( char separator, int base ) const {
    std::stringstream ss;
    if( base == 8 ) {
        ss << std::oct;
    }
    else if( base == 10 ) {
        ss << std::dec;
    }
    else if( base == 16 ) {
        ss << std::hex;
    }
    for( const auto& address : m_address ) {
        ss << address << separator;
    }
    std::string result = ss.str();
    result.pop_back();
    return result;
}

IAddress::Port IAddress::getPort() const {
    return m_port;
}

bool IAddress::operator==( const IAddress& other ) const {
    return m_address == other.m_address && m_port == other.m_port;
}

bool IAddress::operator!=( const IAddress& other ) const {
    return !this->operator==( other );
}

void IAddress::setPort( IAddress::Port port ) {
    m_port = port;
}

Ip4Address::Ip4Address( const Ip4Address::Address& address, IAddress::Port port )
: IAddress( { address[ 0 ], address[ 1 ], address[ 2 ], address[ 3 ] }, port ) {}

Ip4Address::Ip4Address( Ip4Address::Address&& address, IAddress::Port port )
: IAddress(
      { std::move( address[ 0 ] ), std::move( address[ 1 ] ), std::move( address[ 2 ] ), std::move( address[ 3 ] ) },
      port ) {}

IAddress& Ip4Address::operator++() {
    for( int i = getAddressSize() - 1; i != -1; --i ) {
        if( ( ( ++m_address[ i ] ) & 0xFF ) != 0 ) {
            return *this;
        }
        m_address[ i ] = 0;
    }
    return *this;
}

std::string Ip4Address::getAddressString() const {
    return IAddress::getAddressString( '.', 10 );
}

int Ip4Address::getAddressSize() const {
    return AddressSize;
}

Ip4Address::Address Ip4Address::getIp4Address() const {
    return toAddress( getAddress() );
}

Ip4Address::Address Ip4Address::toAddress( const IAddress::BaseAddress& baseAddress ) {
    Address address;
    for( int i = 0; i < baseAddress.size(); ++i ) {
        address[ i ] = static_cast< AddressUnit >( baseAddress[ i ] );
    }
    return address;
}

IAddress::SocketAddress Ip4Address::getSocketAddress() const {
    sockaddr_in addr;
    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *reinterpret_cast< uint32_t* >( getIp4Address().data() );
    addr.sin_port = htons( m_port );
    return { *reinterpret_cast< sockaddr* >( &addr ), sizeof( sockaddr_in ) };
}

bool Ip4Address::setAddress( const IAddress::BaseAddress& address ) {
    if( address.size() > AddressSize ) {
        return false;
    }
    for( const auto& unit : address ) {
        if( unit > 255 ) {
            return false;
        }
    }
    m_address = address;
    return true;
}

Ip6Address::Ip6Address( const Ip6Address::Address& address, IAddress::Port port )
: IAddress( { address[ 0 ],
              address[ 1 ],
              address[ 2 ],
              address[ 3 ],
              address[ 4 ],
              address[ 5 ],
              address[ 6 ],
              address[ 7 ] },
            port ) {}

Ip6Address::Ip6Address( Ip6Address::Address&& address, IAddress::Port port )
: IAddress( { std::move( address[ 0 ] ),
              std::move( address[ 1 ] ),
              std::move( address[ 2 ] ),
              std::move( address[ 3 ] ),
              std::move( address[ 4 ] ),
              std::move( address[ 5 ] ),
              std::move( address[ 6 ] ),
              std::move( address[ 7 ] ) },
            port ) {}

IAddress& Ip6Address::operator++() {
    for( int i = getAddressSize() - 1; i != -1; --i ) {
        if( ( ( ++m_address[ i ] ) & 0xFFFF ) != 0 ) {
            return *this;
        }
        m_address[ i ] = 0;
    }
    return *this;
}

std::string Ip6Address::getAddressString() const {
    return IAddress::getAddressString( ':', 16 );
}

int Ip6Address::getAddressSize() const {
    return AddressSize;
}

Ip6Address::Address Ip6Address::getIp6Address() const {
    return toAddress( getAddress() );
}

Ip6Address::Address Ip6Address::toAddress( const IAddress::BaseAddress& baseAddress ) {
    Address address;
    for( int i = 0; i < baseAddress.size(); ++i ) {
        address[ i ] = static_cast< AddressUnit >( baseAddress[ i ] );
    }
    return address;
}

IAddress::SocketAddress Ip6Address::getSocketAddress() const {
    sockaddr_in6 addr;
    memset( &addr, 0, sizeof( addr ) );
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons( m_port );
    memcpy( addr.sin6_addr.s6_addr, getIp6Address().data(), sizeof( addr.sin6_addr ) );
    return { *reinterpret_cast< sockaddr* >( &addr ), sizeof( sockaddr_in6 ) };
}

bool Ip6Address::setAddress( const IAddress::BaseAddress& address ) {
    if( address.size() > AddressSize ) {
        return false;
    }
    for( const auto& unit : address ) {
        if( unit > 0xffff ) {
            return false;
        }
    }
    m_address = address;
    return true;
}

std::unique_ptr< IAddress > toIpAddress( AddressVersion version, const std::string& addressStr, IAddress::Port port ) {
    auto getAddress = [&addressStr](
                          IAddress::BaseAddress& address, int addressSize, int addressUnitSize, char separator ) {
        std::string::size_type pos = 0;
        for( int i = 0; i < addressSize; ++i, ++pos ) {
            if( pos == std::string::npos ) {
                return false;
            }
            auto oldPos = pos;
            pos = addressStr.find( separator, pos );
            try {
                if( separator == '.' ) {
                    address.push_back( std::stoi( addressStr.substr( oldPos, pos ) ) );
                }
                else if( separator == ':' ) {
                    address.push_back( std::stoi( addressStr.substr( oldPos, pos ), 0, 16 ) );
                }
                else {
                    return false;
                }
            }
            catch( const std::exception& ) {
                return false;
            }
            if( address.back() > addressUnitSize ) {
                return false;
            }
        }
        return true;
    };
    IAddress::BaseAddress address;
    if( version == AddressVersion::IPv4 &&
        getAddress( address, Ip4Address::AddressSize, std::numeric_limits< Ip4Address::AddressUnit >::max(), '.' ) ) {
        return std::make_unique< Ip4Address >( Ip4Address::toAddress( address ), port );
    }
    else if( version == AddressVersion::IPv6 &&
             getAddress(
                 address, Ip6Address::AddressSize, std::numeric_limits< Ip6Address::AddressUnit >::max(), ':' ) ) {
        return std::make_unique< Ip6Address >( Ip6Address::toAddress( address ), port );
    }
    return nullptr;
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
