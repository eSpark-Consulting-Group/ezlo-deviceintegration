#include "networkd/utils/nm_utils.h"

#include "system_utils/system.h"
#include "system_utils/environment.h"
#include "base_utils/string_utils.h"
#include "base_utils/timeutils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"

#include <memory>
#include <functional>
#include <map>
#include <sstream>
#include <fstream>

#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/route.h>

namespace HubPlatform {
namespace Network {
namespace Utils {

bool InterfaceInfo::isUp() {
    return m_ifaFlags & IFF_UP;
}

std::vector< InterfaceInfo > getSystemInterfaces( const std::set< std::string >& names ) {
    std::map< std::string, InterfaceInfo > interfacesMap;
    ifaddrs* ifaddr = nullptr;

    if( getifaddrs( &ifaddr ) != 0 ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to get system network interfaces info" ) );
        return {};
    }

    std::unique_ptr< ifaddrs, std::function< void( ifaddrs* ) > > ifAddresses(
        ifaddr, []( ifaddrs* ifaddr ) { freeifaddrs( ifaddr ); } );

    for( auto* ifa = ifAddresses.get(); ifa; ifa = ifa->ifa_next ) {
        const std::string name( ifa->ifa_name );

        if( !names.empty() && !names.count( name ) )
            continue;

        auto it = interfacesMap.find( name );
        if( it == interfacesMap.end() ) {
            it = interfacesMap.emplace( std::make_pair( name, InterfaceInfo{} ) ).first;

            auto& info = it->second;
            info.m_name = name;
            info.m_ifaFlags = ifa->ifa_flags;

            if( ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET ) {
                struct sockaddr_ll* saddr = (struct sockaddr_ll*) ifa->ifa_addr;
                if( saddr->sll_halen == info.m_macAddress.size() ) {
                    memcpy( info.m_macAddress.data(), saddr->sll_addr, info.m_macAddress.size() );
                }
            }
            else {
                info.m_macAddress.fill( 0 );
            }
        }

        auto& interfaceInfo = it->second;

        if( ifa->ifa_addr && ifa->ifa_netmask &&
            ( ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6 ) ) {
            interfaceInfo.m_addressList.emplace_back();
            auto& addr = interfaceInfo.m_addressList.back();
            addr.m_type =
                ( ifa->ifa_addr->sa_family == AF_INET ) ? InterfaceInfo::Type::IPV4 : InterfaceInfo::Type::IPV6;
            memcpy( &addr.m_ipAddress, ifa->ifa_addr, sizeof( sockaddr ) );
            memcpy( &addr.m_mask, ifa->ifa_netmask, sizeof( sockaddr ) );
        }
    }

    std::vector< InterfaceInfo > interfaces;
    interfaces.reserve( interfacesMap.size() );
    for( auto& element : interfacesMap ) {
        interfaces.push_back( std::move( element.second ) );
    }

    return interfaces;
}

std::string sockadrToString( const sockaddr& sa ) {
    char address[ 64 ] = { '\0' };

    switch( sa.sa_family ) {
    case AF_INET:
        inet_ntop( AF_INET, &reinterpret_cast< const sockaddr_in& >( sa ).sin_addr, address, sizeof( address ) );
        break;

    case AF_INET6:
        inet_ntop( AF_INET6, &reinterpret_cast< const sockaddr_in6& >( sa ).sin6_addr, address, sizeof( address ) );
        break;

    default:;
    }

    if( address[ 0 ] == '\0' ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to convert sockaddr to string" ) );
    }

    return std::string( address );
}

std::string macAddrToString( const InterfaceInfo::MacAddress& address ) {
    std::stringstream ss;
    for( auto& byte : address ) {
        ss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast< int >( byte ) << ":";
    }
    auto result = ss.str();
    result.pop_back();
    return result;
}

// AT commands should be terminated by a CR symbol
std::string purgeCRSymbol( const std::string& atCommand ) {
    if( atCommand.back() == '\r' ) {
        return atCommand.substr( 0, atCommand.size() - 1 );
    }

    return atCommand;
}

std::string getDefaultGatewayIPv4( const std::string& interface ) {
    auto routingTable = System::readFile( "/proc/net/route" );
    if( routingTable.empty() ) {
        LOGGER_WARN( cc::warn( "Unable to read /proc/net/route" ) );
        return {};
    }

    auto lines = StringUtils::splitByStrippedLinesWithText( routingTable );
    if( lines.size() <= 1 ) {
        return {};
    }

    // Iface  Destination  Gateway  Flags  RefCnt  Use  Metric  Mask  MTU  Window  IRTT
    auto header = StringUtils::split( lines.at( 0 ) );
    if( header.size() != 11 || header.at( 0 ) != "Iface" || header.at( 2 ) != "Gateway" || header.at( 3 ) != "Flags" ) {
        LOGGER_WARN( cc::warn( "Not supported /proc/net/route format" ) );
        return {};
    }

    for( size_t i = 0; i != lines.size(); ++i ) {
        auto tokens = StringUtils::split( lines.at( i ) );
        auto& interfaceToken = tokens.at( 0 );
        if( interfaceToken != interface )
            continue;

        unsigned int flags = std::stoul( tokens.at( 3 ), 0, 16 );
        // Ensure interface is up and route is a gateway route
        if( ( flags & RTF_UP ) == 0 || ( flags & RTF_GATEWAY ) == 0 ) {
            continue;
        }

        uint32_t gateway = std::stoul( tokens.at( 2 ), 0, 16 );

        static_assert( sizeof( gateway ) == sizeof( struct in_addr ), "Invalid interface gateway size" );
        return inet_ntoa( *(struct in_addr*) &gateway );
    }

    return {};
}

std::vector< NetstatRecord > getNetstatRecords() {
    int result = 0;
    std::string output;
    std::tie( output, result ) = System::getCommandOutput( "netstat -tunp" );
    auto unpackedExitCode = System::unpackExitCode( result );
    if( unpackedExitCode.exitCode != 0 ) {
        LOGGER_WARN( cc::warn( "Failed to read netstat output:" ) + cc::warn( "code: " ) +
                     cc::num10( unpackedExitCode.exitCode ) + cc::warn( ", err: " ) + cc::note( output ) );
        return {};
    }

    std::vector< NetstatRecord > records;

    auto lines = StringUtils::splitByStrippedLinesWithText( output );
    if( lines.empty() )
        return {};

    auto lineIt = lines.begin();
    ++lineIt;  // Skip header

    for( ; lineIt != lines.end(); ++lineIt ) {
        // Proto Recv-Q Send-Q Local-Address Foreign-Address State PID/Program-name
        auto columns = StringUtils::split( *lineIt );
        if( columns.size() != 7 )
            continue;
        NetstatRecord record;
        record.proto = columns.at( 0 );
        record.localAddress = columns.at( 3 );
        record.foreignAddress = columns.at( 4 );
        record.state = columns.at( 5 );

        auto& pidAndProg = columns.at( 6 );
        auto pos = pidAndProg.find( '/' );
        if( pos == std::string::npos )
            continue;
        record.pid = std::string( pidAndProg.data(), pos );
        record.program = std::string( pidAndProg.data() + pos + 1 );

        records.emplace_back( std::move( record ) );
    }

    return records;
}

void restartNetworkService() {
#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ )
    auto errorReport = System::getCommandOutputWithErrorReport( "/etc/init.d/network restart" );
    if( errorReport.exitCode != 0 ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to restart network service" ) + cc::num10( errorReport.exitCode ) +
                         cc::warn( ", err: " ) + cc::note( errorReport.output ) );
    }
#endif
}

void logWirelessConfig() {
    try {
        std::string content = "==========" + TimeUtils::getCurrentIsoDate() + "==========\n";
        content += System::readFile( "/etc/config/wireless" );
        content += "\n";

        std::string destination = Environment::getLogsFolder() + "/wireless_config_history.log";

        std::ofstream file( destination, std::ios::app );
        if( !file.is_open() ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to dump wireless config file: unable to open file: " + destination ) );
            return;
        }

        file << content;
        file.flush();
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( "Unable to dump wireless config: " + e.what() );
    }
}

}  // namespace Utils
}  // namespace Network
}  // namespace HubPlatform
