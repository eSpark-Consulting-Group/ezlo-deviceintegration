#include "networkd/monitoring/nm_ping_parsers.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "base_utils/string_utils.h"
#include "common_utils/exception_utils.h"
#include "system_utils/environment.h"

#include <chrono>
#include <utility>

namespace HubPlatform {
namespace Network {
namespace PingParsers {

namespace {

void ensureCondition( bool condition, const std::string& conditionLabel ) {
    if( !condition ) {
        throw std::runtime_error( "Condition not met: " + conditionLabel );
    }
}

}  // namespace

Result parsePingOutput( const std::string& pingOutput ) {
    if( Environment::isPhysicalHub() ) {
        return parseOpenWrtPingOutput( pingOutput );
    }
    return parseDebianPingOutput( pingOutput );
}

Result parseOpenWrtPingOutput( const std::string& pingOutput ) {
    // Expected string:
    // 64 bytes from 8.8.8.8: seq=2 ttl=112 time=25.401 ms
    // 64 bytes from 8.8.8.8: seq=3 ttl=112 time=25.355 ms
    // --- 8.8.8.8 ping statistics ---
    // 4 packets transmitted, 4 packets received, 0% packet loss
    // round-trip min/avg/max = 25.319/25.417/25.613 ms

    // NOTE: round trip line will be missing if all packets lost

    try {
        Result result;

        auto lines = StringUtils::splitByStrippedLinesWithText( pingOutput );
        ensureCondition( !lines.empty(), "Invalid lines number" );

        std::string transmittedLine;
        for( auto it = lines.rbegin(); it != lines.rend(); ++it ) {
            if( it->find( "transmitted" ) != std::string::npos ) {
                transmittedLine = *it;
            }
        }
        ensureCondition( !transmittedLine.empty(), "Missing 'transmitted' field" );

        std::string roundTripLine;
        for( auto it = lines.rbegin(); it != lines.rend(); ++it ) {
            if( it->find( "round-trip" ) != std::string::npos ) {
                roundTripLine = *it;
            }
        }

        {
            auto tokens = StringUtils::split( transmittedLine );
            ensureCondition( tokens.size() == 9, "Invalid 'transmitted' line size" );
            result.packetsSent = std::stoi( tokens.at( 0 ) );
            result.packetsReceived = std::stoi( tokens.at( 3 ) );
        }

        if( !roundTripLine.empty() ) {
            auto tokens = StringUtils::split( roundTripLine );
            ensureCondition( tokens.size() == 5, "Invalid 'round-trip' line size" );

            auto values = StringUtils::split( tokens.at( 3 ), "/" );
            ensureCondition( values.size() == 3, "Invalid 'round-trip' values" );

            result.roundTripMin = std::stod( values.at( 0 ) );
            result.roundTripAvg = std::stod( values.at( 1 ) );
            result.roundTripMax = std::stod( values.at( 2 ) );
        }

        return result;
    }
    catch( const std::exception& e ) {
        LOGGER_WARN( cc::warn( "Unable to parse ping output, error: " ) + cc::note( CommonUtils::whatError( e ) ) +
                     cc::warn( ", output: " ) + cc::note( pingOutput ) );
    }

    return {};
}

Result parseDebianPingOutput( const std::string& pingOutput ) {
    // Expected string:
    // PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
    // 64 bytes from 8.8.8.8: icmp_seq=1 ttl=112 time=26.2 ms
    // 64 bytes from 8.8.8.8: icmp_seq=2 ttl=112 time=25.8 ms
    // 64 bytes from 8.8.8.8: icmp_seq=3 ttl=112 time=25.9 ms
    // 64 bytes from 8.8.8.8: icmp_seq=4 ttl=112 time=26.1 ms
    // 64 bytes from 8.8.8.8: icmp_seq=5 ttl=112 time=25.8 ms
    // --- 8.8.8.8 ping statistics ---
    // 5 packets transmitted, 5 received, 0% packet loss, time 4004ms
    // rtt min/avg/max/mdev = 25.828/25.961/26.241/0.162 ms

    // NOTE: round trip line will be missing if all packets lost

    try {
        Result result;

        auto lines = StringUtils::splitByStrippedLinesWithText( pingOutput );
        ensureCondition( !lines.empty(), "Invalid lines number" );

        std::string transmittedLine;
        for( auto it = lines.rbegin(); it != lines.rend(); ++it ) {
            if( it->find( "transmitted" ) != std::string::npos ) {
                transmittedLine = *it;
            }
        }
        ensureCondition( !transmittedLine.empty(), "Missing 'transmitted' field" );

        std::string roundTripLine;
        for( auto it = lines.rbegin(); it != lines.rend(); ++it ) {
            if( it->find( "min/avg/max/mdev" ) != std::string::npos ) {
                roundTripLine = *it;
            }
        }

        {
            auto tokens = StringUtils::split( transmittedLine );
            ensureCondition( tokens.size() == 10, "Invalid 'transmitted' line size" );
            result.packetsSent = std::stoi( tokens.at( 0 ) );
            result.packetsReceived = std::stoi( tokens.at( 3 ) );
        }

        if( !roundTripLine.empty() ) {
            auto tokens = StringUtils::split( roundTripLine );
            ensureCondition( tokens.size() == 5, "Invalid round trip line size" );

            auto values = StringUtils::split( tokens.at( 3 ), "/" );
            ensureCondition( values.size() == 4, "Invalid round trip values" );

            result.roundTripMin = std::stod( values.at( 0 ) );
            result.roundTripAvg = std::stod( values.at( 1 ) );
            result.roundTripMax = std::stod( values.at( 2 ) );
        }

        return result;
    }
    catch( const std::exception& e ) {
        LOGGER_WARN( cc::warn( "Unable to parse ping output, error: " ) + cc::note( CommonUtils::whatError( e ) ) +
                     cc::warn( ", output: " ) + cc::note( pingOutput ) );
    }

    return {};
}

}  // namespace PingParsers
}  // namespace Network
}  // namespace HubPlatform
