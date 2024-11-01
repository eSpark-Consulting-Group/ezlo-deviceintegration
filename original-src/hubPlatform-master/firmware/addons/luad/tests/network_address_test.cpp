#include "luad/modules/network/network_address.h"
#include "luad/modules/network/network_address_container.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

class NetworkAddressTestSuite : public testing::Test {};

class NetworkAddressContainerTestSuite : public testing::Test {};

TEST_F( NetworkAddressTestSuite, ip4AddressGetters ) {
    LuaNetwork::Ip4Address ipAddress( { 192, 168, 0, 1 }, 2020 );
    EXPECT_EQ( ipAddress.getAddressSize(), 4 );
    EXPECT_EQ( ipAddress.getAddressString(), "192.168.0.1" );
    EXPECT_EQ( ipAddress.getIp4Address(), LuaNetwork::Ip4Address::Address( { 192, 168, 0, 1 } ) );
    EXPECT_EQ( ipAddress.getAddress(), LuaNetwork::IAddress::BaseAddress( { 192, 168, 0, 1 } ) );
    EXPECT_EQ( ipAddress.getPort(), 2020 );
    sockaddr_in addr;
    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0x0100A8C0;
    addr.sin_port = htons( 2020 );
    auto socketAddress = ipAddress.getSocketAddress();
    sockaddr& expectedAddr = *reinterpret_cast< sockaddr* >( &addr );
    for( int i = 0; i < sizeof( addr.sin_addr.s_addr ); ++i ) {
        EXPECT_EQ( socketAddress.address.sa_data[ i ], expectedAddr.sa_data[ i ] );
    }
    EXPECT_EQ( socketAddress.address.sa_family, expectedAddr.sa_family );
    EXPECT_EQ( socketAddress.size, sizeof( sockaddr_in ) );
}

TEST_F( NetworkAddressTestSuite, ip6AddressGetters ) {
    LuaNetwork::Ip6Address ipAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 );
    EXPECT_EQ( ipAddress.getAddressSize(), 8 );
    EXPECT_EQ( ipAddress.getAddressString(), "2001:db8:85a3:8d3:1319:8a2e:370:7348" );
    EXPECT_EQ( ipAddress.getIp6Address(),
               LuaNetwork::Ip6Address::Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 } ) );
    EXPECT_EQ( ipAddress.getAddress(),
               LuaNetwork::IAddress::BaseAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 } ) );
    EXPECT_EQ( ipAddress.getPort(), 2020 );
    const uint16_t addressArray[] = { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 };
    sockaddr_in6 addr;
    memset( &addr, 0, sizeof( addr ) );
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons( 2020 );
    memcpy( addr.sin6_addr.s6_addr, addressArray, sizeof( addr.sin6_addr ) );
    auto socketAddress = ipAddress.getSocketAddress();
    sockaddr& expectedAddr = *reinterpret_cast< sockaddr* >( &addr );
    for( int i = 0; i < sizeof( socketAddress.address.sa_data ); ++i ) {
        EXPECT_EQ( socketAddress.address.sa_data[ i ], expectedAddr.sa_data[ i ] );
    }
    EXPECT_EQ( socketAddress.address.sa_family, expectedAddr.sa_family );
    EXPECT_EQ( socketAddress.size, sizeof( sockaddr_in6 ) );
}

TEST_F( NetworkAddressTestSuite, ip4AddressSetters ) {
    using namespace LuaNetwork;
    IAddress::BaseAddress expectedAddress = { 192, 168, 0, 1 };
    Ip4Address ipAddress( { 192, 168, 0, 1 }, 2020 );
    ipAddress.setPort( 8008 );
    EXPECT_EQ( ipAddress.getPort(), 8008 );
    EXPECT_FALSE( ipAddress.setAddress( { 192, 168, 0, 1, 1 } ) );
    EXPECT_EQ( ipAddress.getAddress(), expectedAddress );
    EXPECT_FALSE( ipAddress.setAddress( { 192, 168, 0, 300 } ) );
    EXPECT_EQ( ipAddress.getAddress(), expectedAddress );
    EXPECT_TRUE( ipAddress.setAddress( { 0, 34, 22, 100 } ) );
    EXPECT_EQ( ipAddress.getAddress(), IAddress::BaseAddress( { 0, 34, 22, 100 } ) );
}

TEST_F( NetworkAddressTestSuite, ip6AddressSetters ) {
    using namespace LuaNetwork;
    IAddress::BaseAddress expectedAddress = { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 };
    Ip6Address ipAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 );
    ipAddress.setPort( 8008 );
    EXPECT_EQ( ipAddress.getPort(), 8008 );
    EXPECT_FALSE( ipAddress.setAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348, 0x7348 } ) );
    EXPECT_EQ( ipAddress.getAddress(), expectedAddress );
    EXPECT_FALSE( ipAddress.setAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x237348 } ) );
    EXPECT_EQ( ipAddress.getAddress(), expectedAddress );
    EXPECT_TRUE( ipAddress.setAddress( { 0x2001, 0x0, 0x8, 0x4, 0x19, 0x2e, 0x70, 0x7348 } ) );
    EXPECT_EQ( ipAddress.getAddress(), IAddress::BaseAddress( { 0x2001, 0x0, 0x8, 0x4, 0x19, 0x2e, 0x70, 0x7348 } ) );
}

TEST_F( NetworkAddressTestSuite, ip4AddressIncrement ) {
    using namespace LuaNetwork;
    Ip4Address ipAddress( { 192, 168, 0, 1 }, 2020 );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "192.168.0.2" );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "192.168.0.3" );
    ipAddress.setAddress( { 192, 255, 0, 255 } );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "192.255.1.0" );

    ipAddress.setAddress( { 192, 168, 255, 255 } );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "192.169.0.0" );

    ipAddress.setAddress( { 192, 255, 255, 255 } );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "193.0.0.0" );
}

TEST_F( NetworkAddressTestSuite, ip6AddressIncrement ) {
    using namespace LuaNetwork;
    Ip6Address ipAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "2001:db8:85a3:8d3:1319:8a2e:370:7349" );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "2001:db8:85a3:8d3:1319:8a2e:370:734a" );
    ipAddress.setAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0xffff } );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "2001:db8:85a3:8d3:1319:8a2e:371:0" );
    ipAddress.setAddress( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x00ff } );
    ++ipAddress;
    EXPECT_EQ( ipAddress.getAddressString(), "2001:db8:85a3:8d3:1319:8a2e:370:100" );
}

TEST_F( NetworkAddressTestSuite, ip4AddressEqual ) {
    using namespace LuaNetwork;
    EXPECT_EQ( Ip4Address( { 192, 168, 0, 1 }, 2020 ), Ip4Address( { 192, 168, 0, 1 }, 2020 ) );
    EXPECT_EQ( Ip4Address( { 192, 168, 100, 1 }, 2021 ), Ip4Address( { 192, 168, 100, 1 }, 2021 ) );
    EXPECT_NE( Ip4Address( { 192, 168, 100, 2 }, 2021 ), Ip4Address( { 192, 168, 100, 1 }, 2021 ) );
    EXPECT_NE( Ip4Address( { 192, 168, 100, 1 }, 2022 ), Ip4Address( { 192, 168, 100, 1 }, 2021 ) );
}

TEST_F( NetworkAddressTestSuite, ip6AddressEqual ) {
    using namespace LuaNetwork;
    EXPECT_EQ( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_EQ( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x8a2e }, 2022 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x8a2e }, 2022 ) );
    EXPECT_NE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x8a23 }, 2022 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x8a2e }, 2022 ) );
    EXPECT_NE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x8a23 }, 2023 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x8a2e }, 2022 ) );
}

TEST_F( NetworkAddressTestSuite, ip4AddressLess ) {
    using namespace LuaNetwork;
    EXPECT_LT( Ip4Address( { 192, 168, 0, 1 }, 2020 ), Ip4Address( { 192, 168, 0, 2 }, 2020 ) );
    EXPECT_LT( Ip4Address( { 192, 168, 0, 1 }, 2020 ), Ip4Address( { 192, 169, 0, 1 }, 2020 ) );
    EXPECT_LT( Ip4Address( { 192, 168, 0, 1 }, 2020 ), Ip4Address( { 193, 168, 0, 1 }, 2020 ) );
    EXPECT_FALSE( Ip4Address( { 192, 168, 0, 1 }, 2020 ) < Ip4Address( { 192, 168, 0, 1 }, 2020 ) );
    EXPECT_FALSE( Ip4Address( { 192, 169, 0, 1 }, 2020 ) < Ip4Address( { 192, 168, 0, 1 }, 2020 ) );
    EXPECT_FALSE( Ip4Address( { 193, 168, 0, 1 }, 2020 ) < Ip4Address( { 192, 168, 0, 1 }, 2020 ) );
}

TEST_F( NetworkAddressTestSuite, ip6AddressLess ) {
    using namespace LuaNetwork;
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7349 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x371, 0x7348 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2f, 0x370, 0x7348 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x131a, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d4, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb8, 0x85a4, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2001, 0xdb9, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_LT( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ),
               Ip6Address( { 0x2002, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7349 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x371, 0x7348 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2f, 0x370, 0x7348 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x131a, 0x8a2e, 0x370, 0x7348 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d4, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2001, 0xdb8, 0x85a4, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
    EXPECT_FALSE( Ip6Address( { 0x2002, 0xdb9, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) <
                  Ip6Address( { 0x2001, 0xdb8, 0x85a3, 0x8d3, 0x1319, 0x8a2e, 0x370, 0x7348 }, 2020 ) );
}

TEST_F( NetworkAddressContainerTestSuite, AddressList ) {
    using namespace LuaNetwork;
    AddressList::Addresses addresess;
    addresess.emplace_back( std::make_unique< Ip4Address >( Ip4Address::Address( { 192, 168, 0, 1 } ), 0 ) );
    addresess.emplace_back( std::make_unique< Ip4Address >( Ip4Address::Address( { 192, 168, 0, 2 } ), 0 ) );
    addresess.emplace_back( std::make_unique< Ip4Address >( Ip4Address::Address( { 192, 168, 0, 3 } ), 0 ) );
    addresess.emplace_back( std::make_unique< Ip4Address >( Ip4Address::Address( { 192, 168, 0, 4 } ), 0 ) );
    AddressList addressList( std::move( addresess ) );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getLast(), Ip4Address( { 192, 168, 0, 4 }, 0 ) );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 1 }, 0 ) );
    EXPECT_TRUE( addressList.next() );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 2 }, 0 ) );
    EXPECT_TRUE( addressList.next() );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 3 }, 0 ) );
    EXPECT_TRUE( addressList.next() );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 4 }, 0 ) );
    EXPECT_FALSE( addressList.next() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 4 }, 0 ) );
    EXPECT_TRUE( addressList.isFinished() );

    addressList.setFirst();
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 1 }, 0 ) );
    EXPECT_TRUE( addressList.next() );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 2 }, 0 ) );
    EXPECT_TRUE( addressList.next() );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 3 }, 0 ) );
    EXPECT_TRUE( addressList.next() );
    EXPECT_FALSE( addressList.isFinished() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 4 }, 0 ) );
    EXPECT_FALSE( addressList.next() );
    EXPECT_EQ( *addressList.getCurrent(), Ip4Address( { 192, 168, 0, 4 }, 0 ) );
    EXPECT_TRUE( addressList.isFinished() );
}

TEST_F( NetworkAddressContainerTestSuite, AddressRange ) {
    using namespace LuaNetwork;
    AddressRange addressRange( std::make_unique< Ip4Address >( Ip4Address::Address( { 192, 168, 0, 250 } ), 0 ),
                               std::make_unique< Ip4Address >( Ip4Address::Address( { 192, 168, 1, 0 } ), 0 ) );
    EXPECT_FALSE( addressRange.isFinished() );
    EXPECT_EQ( addressRange.getLast()->getAddressString(), "192.168.1.0" );
    EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.0.250" );
    for( uint8_t i = 250; i != 255; ++i, addressRange.next() ) {
        EXPECT_FALSE( addressRange.isFinished() );
        EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.0." + std::to_string( i ) );
    }
    addressRange.next();
    EXPECT_FALSE( addressRange.isFinished() );
    EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.1.0" );
    addressRange.next();
    EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.1.0" );
    EXPECT_TRUE( addressRange.isFinished() );

    addressRange.setFirst();
    EXPECT_FALSE( addressRange.isFinished() );
    EXPECT_EQ( addressRange.getLast()->getAddressString(), "192.168.1.0" );
    EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.0.250" );
    for( uint8_t i = 250; i != 255; ++i, addressRange.next() ) {
        EXPECT_FALSE( addressRange.isFinished() );
        EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.0." + std::to_string( i ) );
    }
    addressRange.next();
    EXPECT_FALSE( addressRange.isFinished() );
    EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.1.0" );
    addressRange.next();
    EXPECT_EQ( addressRange.getCurrent()->getAddressString(), "192.168.1.0" );
    EXPECT_TRUE( addressRange.isFinished() );
}

}  // namespace UT
}  // namespace HubPlatform
