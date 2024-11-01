#include "networkd/monitoring/nm_monitor_utils.h"
#include "networkd/resources/nm_resources.h"

#include "system_utils/system.h"
#include "common_utils/exception_utils.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

// 192.168.11.41 -> 0xC0A80B29, 0xFFFFFFFF
// 192.168.11.41\/23 -> 0xC0A80B29, 0xFFFFFE00
std::pair< uint32_t, uint32_t > parseIpCidr( const std::string& ipCidr ) {
    int a, b, c, d, bits;
    uint32_t addr = 0, mask = -1;

    int scanned = sscanf( ipCidr.c_str(), "%d.%d.%d.%d/%d", &a, &b, &c, &d, &bits );
    if( scanned >= 4 ) {
        addr = a << 24;
        addr |= b << 16;
        addr |= c << 8;
        addr |= d;
    }

    if( scanned == 5 ) {
        if( bits == 0 ) {
            mask = 0;
        }
        else {
            mask = uint32_t( -1 ) << ( 32 - bits );
        }
    }

    return std::make_pair( addr, mask );
}

// 23 -> "255.255.254.0"
std::string generateMaskString( int bits ) {
    uint32_t mask = ( bits == 0 ) ? 0 : uint32_t( -1 ) << ( 32 - bits );
    return std::to_string( ( mask >> 24 ) & 0xFF ) + "." + std::to_string( ( mask >> 16 ) & 0xFF ) + "." +
           std::to_string( ( mask >> 8 ) & 0xFF ) + "." + std::to_string( ( mask >> 0 ) & 0xFF );
}

std::string getInterfaceMacAddress( const std::string& interfaceName ) {
    std::string macAddress;
    if( interfaceName.empty() ) {
        return macAddress;
    }

    /* mobile interfaces have a ppp connection type which has no mac address
     * generally we check a mac address for a device name (physical interface name)
     * rather than for a interface one (virtual interface name) */
    if( interfaceName.find( Resources::MobileBaseDeviceName ) != std::string::npos ) {
        return "ppp";
    }

    int result = 0;
    std::tie( macAddress, result ) =
        System::getCommandOutput( ( "ip -br link show " + interfaceName + " | awk '{print $3}'" ).c_str() );
    if( result == 0 && !macAddress.empty() ) {
        macAddress.resize( macAddress.size() - 1 );
        return macAddress;
    }
    return {};
}

Json getActualNetworkIfaceConfig( const std::string& name ) {
    using namespace Resources::InterfaceInfo;
    std::string output;
    int exitCode = 0;
    std::tie( output, exitCode ) = System::getCommandOutput( ( "ifstatus " + name ).c_str() );
    auto config = JSON_EMPTY_OBJ;
    try {
        config = parseVirtualInterfaceStatus( output );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception while parsing network interface status: interface: " ) +
                          cc::note( name ) + cc::error( CommonUtils::whatError( e ) ) );
        return {};
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Unknown exception while parsing network interface status: interface: " ) +
                          cc::note( name ) );
        return {};
    }

    auto id = json_get_str( config, Fields::Id );
    std::string macAddress = getInterfaceMacAddress( id );
    if( !macAddress.empty() ) {
        config[ Fields::HwAddress ] = macAddress;
    }
    return config;
}

Json getNetworkInterfaceStatus( const std::string& name ) {
    using namespace Resources::InterfaceInfo;
    std::string output;
    int exitCode = 0;
    std::tie( output, exitCode ) = System::getCommandOutput( ( "ifstatus " + name ).c_str() );

    Json jsonStatus;
    try {
        if( parseJson( output, jsonStatus ) ) {
            return jsonStatus;
        }
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( "error: " + ex.what() );
        return JSON_EMPTY_OBJ;
    }
    return JSON_EMPTY_OBJ;
}

Json parseVirtualInterfaceStatus( const std::string& status ) {
    using namespace Resources::InterfaceInfo;
    Json result;
    result[ Fields::Status ] = Statuses::Down;

    Json jsonStatus;
    if( !parseJson( status, jsonStatus ) || jsonStatus.is_null() ) {
        return result;
    }

    auto protoIt = jsonStatus.find( "proto" );
    if( protoIt == jsonStatus.end() ) {
        return result;
    }

    std::string deviceFieldName;

    if( ( *protoIt ).get< std::string >() == "3g" ) {
        deviceFieldName = "l3_device";
    }
    else {
        deviceFieldName = "device";
    }

    auto deviceIt = jsonStatus.find( deviceFieldName );

    if( deviceIt == jsonStatus.end() || !deviceIt.value().is_string() ) {
        return result;
    }

    result[ Fields::Id ] = std::move( *deviceIt );
    if( json_get_bool( jsonStatus, "up" ) ) {
        result[ Fields::Status ] = Statuses::Up;
    }

    const auto routesIt = jsonStatus.find( "route" );

    auto ipv4It = jsonStatus.find( "ipv4-address" );
    if( ipv4It != jsonStatus.end() && ipv4It.value().is_array() && !ipv4It.value().empty() ) {
        const auto& ipv4Item = ipv4It.value().front();

        auto maskIt = ipv4Item.find( "mask" );
        auto addressIt = ipv4Item.find( "address" );

        if( maskIt != ipv4Item.end() && addressIt != ipv4Item.end() && addressIt.value().is_string() ) {
            auto& ipv4 = result[ Fields::Ipv4 ];
            ipv4[ Fields::Mask ] = generateMaskString( maskIt.value().get< int >() );
            ipv4[ Fields::Ip ] = *addressIt;
            ipv4[ Fields::Mode ] = *protoIt;

            if( routesIt != jsonStatus.end() && routesIt.value().is_array() ) {
                uint32_t addressIp;
                std::tie( addressIp, std::ignore ) = parseIpCidr( addressIt.value().get< std::string >() );

                const auto routeIt =
                    std::find_if( routesIt.value().begin(), routesIt.value().end(), [&]( const Json& route ) {
                        const auto sourceIt = route.find( "source" );

                        uint32_t sourceIp, sourceMask;
                        std::tie( sourceIp, sourceMask ) = parseIpCidr( sourceIt.value().get< std::string >() );
                        return sourceIt != route.end() && sourceIt.value().is_string() &&
                               ( sourceIp & sourceMask ) == ( addressIp & sourceMask );
                    } );

                if( routeIt != routesIt.value().end() ) {
                    auto gatewayIt = routeIt.value().find( "nexthop" );
                    if( gatewayIt != routeIt.value().end() ) {
                        ipv4[ Fields::Gateway ] = std::move( *gatewayIt );
                    }
                }
            }
        }
    }

    auto ipv6It = jsonStatus.find( "ipv6-address" );
    if( ipv6It != jsonStatus.end() && ipv6It.value().is_array() && !ipv6It.value().empty() ) {
        auto& ipv6 = result[ Fields::Ipv6 ];
        ipv6[ Fields::Mode ] = *protoIt;

        auto& addresses = ipv6[ Fields::Addresses ] = Json::value_t::array;

        for( Json& ipv6Item : *ipv6It ) {
            auto addressIt = ipv6Item.find( "address" );
            auto maskIt = ipv6Item.find( "mask" );
            if( addressIt != ipv6Item.end() && maskIt != ipv6Item.end() ) {
                Json convertedItem;
                convertedItem[ Fields::Ip ] = std::move( *addressIt );
                convertedItem[ Fields::PrefixLen ] = std::move( *maskIt );
                addresses.push_back( std::move( convertedItem ) );
            }
        }
    }

    JsonArray ipv4Dns = Json::value_t::array;
    JsonArray ipv6Dns = Json::value_t::array;

    auto dnsIt = jsonStatus.find( "dns-server" );
    if( dnsIt != jsonStatus.end() && dnsIt.value().is_array() ) {
        for( Json& address : *dnsIt ) {
            if( auto str = address.get< const std::string* >() ) {
                if( str->find( '.' ) != std::string::npos ) {
                    ipv4Dns.push_back( std::move( address ) );
                }
                else if( str->find( ':' ) != std::string::npos ) {
                    ipv6Dns.push_back( std::move( address ) );
                }
            }
        }
    }

    if( !ipv4Dns.empty() ) {
        result[ Fields::Ipv4 ][ Fields::Dns ] = std::move( ipv4Dns );
    }

    if( !ipv6Dns.empty() ) {
        result[ Fields::Ipv6 ][ Fields::Dns ] = std::move( ipv6Dns );
    }

    return result;
}

}  // namespace Network
}  // namespace HubPlatform
