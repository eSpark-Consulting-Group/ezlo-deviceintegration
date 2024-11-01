#include "networkd/routes/nm_openwrt_routes_manager.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/utils/nm_utils.h"

#include "system_utils/system.h"
#include "base_utils/string_utils.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <algorithm>
#include <limits>
#include <thread>

namespace HubPlatform {
namespace Network {

namespace {
namespace Impl {

enum class ExitStatus : int { OK = 0, SYNTAX_ERROR = 1, KERNEL_ERROR = 2 };

/** Converts ip command exit code to string.
 * @params exitCode - exit code of a child process, unpacked. See man ip(8) for available exit codes.
 *      One of: 0 - ok, 1 - syntax error, 2 - kernel error */
std::string exitCodeToString( int exitCode ) {
    std::string error;

    switch( static_cast< Impl::ExitStatus >( exitCode ) ) {
    case Impl::ExitStatus::OK: {
        error = "success";
        break;
    }
    case Impl::ExitStatus::SYNTAX_ERROR: {
        error = "syntax error";
        break;
    }
    case Impl::ExitStatus::KERNEL_ERROR: {
        error = "kernel error";
        break;
    }
    default:
        error = "unknown error: " + std::to_string( exitCode );
    }
    return error;
}

int toInt( Impl::ExitStatus status ) {
    return static_cast< int >( status );
}

bool applyIpRouteAction( const std::string& action, int attempts, std::chrono::milliseconds repeatInterval ) {
    int result = 0;
    std::string output;

    if( attempts < 1 )
        attempts = 1;

    LOGGER_INFO( cc::info( "Applying route action: " ) + cc::note( action ) );

    for( int attempt = 1; attempt <= attempts; attempt++ ) {
        std::tie( output, result ) = System::getCommandOutput( action.c_str() );
        auto unpackedExitCode = System::unpackExitCode( result );
        if( unpackedExitCode.exitCode == 0 ) {
            LOGGER_TRACE( cc::info( "Action applied successfully, attempt: " ) + cc::num10( attempt ) );
            return true;
        }
        else {
            std::string error = exitCodeToString( unpackedExitCode.exitCode );

            LOGGER_WARN( cc::warn( "Failed to apply route action: attempt N:" ) + cc::num10( attempt + 1 ) +
                         cc::warn( ", code: " ) + cc::num10( unpackedExitCode.exitCode ) +
                         cc::warn( ", error type: " + error ) + cc::warn( ", err: " ) + cc::note( output ) );

            if( unpackedExitCode.exitCode == toInt( ExitStatus::SYNTAX_ERROR ) ) {
                LOGGER_ERROR( cc::error( "IP Command syntax error" ) );
                break;
            }
            else if( unpackedExitCode.exitCode == toInt( ExitStatus::KERNEL_ERROR ) ) {
                LOGGER_INFO( cc::info( "Wait and repeat" ) );
                std::this_thread::sleep_for( repeatInterval );
            }
        }
    }
    return false;
}

std::string makeRouteAddAction( const std::string& gateway, const std::string& device ) {
    return ( "ip route add default via " + gateway + " dev " + device );
}

std::string makeRouteAddActionWithMetric( const std::string& gateway,
                                          const std::string& device,
                                          const std::string& metric ) {
    return ( "ip route add default via " + gateway + " dev " + device + " metric " + metric );
}

std::string makeRouteReplaceAction( const std::string& gateway, const std::string& device ) {
    return ( "ip route replace default via " + gateway + " dev " + device );
}

std::string makeRouteDeleteAction( const std::string& route ) {
    return ( "ip route del " + route );
}

std::vector< Json > sortInterfacesByMetric( const std::vector< Json >& interfaces ) {
    using namespace Resources::InterfaceInfo;
    auto compareObj = []( const Json& left, const Json& right ) {
        auto leftPriority = json_get_int( left, Fields::Metric, std::numeric_limits< int >::max() );
        auto rightPriority = json_get_int( right, Fields::Metric, std::numeric_limits< int >::max() );

        return leftPriority < rightPriority;  // higher priority, lower metric value
    };

    auto sortedInterfaces = interfaces;
    std::sort( sortedInterfaces.begin(), sortedInterfaces.end(), compareObj );
    return sortedInterfaces;
}

}  // namespace Impl
}  // namespace

class OpenwrtRoutesActionHandler : public OpenwrtRoutesManager::ActionHandlerInterface {
public:
    std::string readRoutesTable() override;
    bool applyRouteAction( const std::string& action ) override;
};

std::string OpenwrtRoutesActionHandler::readRoutesTable() {
    int result = 0;
    std::string output;

    std::tie( output, result ) = System::getCommandOutput( "ip route" );
    auto unpackedExitCode = System::unpackExitCode( result );
    if( unpackedExitCode.exitCode == 0 ) {
        return output;
    }

    std::string error = Impl::exitCodeToString( unpackedExitCode.exitCode );
    LOGGER_WARN( cc::warn( "Failed to read network routes table: " ) + cc::warn( "exit status: " ) +
                 cc::num10( result ) + cc::warn( ", exit status reason: " ) + cc::note( unpackedExitCode.error ) +
                 cc::warn( ", code: " ) + cc::num10( unpackedExitCode.exitCode ) +
                 cc::warn( ", error type: " + error ) + cc::warn( ", err: " ) + cc::note( output ) );
    return {};
}

bool OpenwrtRoutesActionHandler::applyRouteAction( const std::string& action ) {
    int attempts = 1;
    std::chrono::milliseconds repeatInterval{ 500 };
    return Impl::applyIpRouteAction( action, attempts, repeatInterval );
}

OpenwrtRoutesManager::OpenwrtRoutesManager()
: m_actionHandler( std::make_unique< OpenwrtRoutesActionHandler >() ) {}

OpenwrtRoutesManager::OpenwrtRoutesManager( std::unique_ptr< ActionHandlerInterface > actionHandler )
: m_actionHandler( std::move( actionHandler ) ) {}

// NOTE: function returns default route that was set manually
std::string OpenwrtRoutesManager::getDefaultRouteDevice() {
    const auto routes = readRoutes();
    return getDefaultRouteDevice( routes );
}

std::string OpenwrtRoutesManager::checkAndFixRoutes( const std::vector< Json >& interfaces ) {
    using namespace Resources::InterfaceInfo;

    /* sort interfaces by their priorities in descending order
     * if an interface has no priority specified, it's implied as the lowest one */
    auto sortedInterfaces = Impl::sortInterfacesByMetric( interfaces );

    const auto routes = readRoutes();
    std::string currentDevice = getDefaultRouteDevice( routes );
    bool isCurrentDeviceValid = false;

    if( currentDevice.empty() ) {
        LOGGER_WARN( cc::warn( "No interface is suitable for default route" ) );
    }

    for( auto& interface : sortedInterfaces ) {
        const auto device = json_get_str( interface, Fields::Id );
        const std::string metric = std::to_string( json_get_int( interface, Fields::Metric, 0 ) );
        const bool isUp = json_get_str( interface, Fields::Status ) == Statuses::Up;
        const bool internetAvailable = json_get_bool( interface, Fields::InternetAvailable );
        const auto& ipv4 = interface[ Fields::Ipv4 ];
        const std::string gateway = json_get_str( ipv4, Fields::Gateway );

        if( device.empty() ) {
            LOGGER_ERROR( cc::error( "Interface with empty device value provided in interfaces list" ) );
            continue;
        }

        const bool isActiveInterface = isUp && !gateway.empty();

        if( isActiveInterface && internetAvailable ) {
            if( currentDevice.empty() ) {
                std::string action = Impl::makeRouteAddAction( gateway, device );
                if( m_actionHandler->applyRouteAction( action ) ) {
                    currentDevice = device;
                    isCurrentDeviceValid = true;
                }
            }
            else if( currentDevice == device ) {
                isCurrentDeviceValid = true;
            }
            else if( !isCurrentDeviceValid ) {
                std::string action = Impl::makeRouteReplaceAction( gateway, device );
                if( m_actionHandler->applyRouteAction( action ) ) {
                    currentDevice = device;
                    isCurrentDeviceValid = true;
                }
            }
        }

        bool hasRouteWithMetric = false;

        for( auto& route : routes ) {
            if( !route.isDefault || device != route.device )
                continue;

            // primary route
            if( route.proto.empty() && route.metric.empty() )
                continue;

            if( route.metric == metric ) {
                hasRouteWithMetric = true;
            }
            else if( route.proto.empty() ) {  // Any other manually created route
                std::string action = Impl::makeRouteDeleteAction( route.routeString );
                m_actionHandler->applyRouteAction( action );
            }
        }

        if( isActiveInterface && !hasRouteWithMetric ) {
            std::string action = Impl::makeRouteAddActionWithMetric( gateway, device, metric );
            m_actionHandler->applyRouteAction( action );
        }
    }

    return currentDevice;
}

std::string OpenwrtRoutesManager::getDefaultRouteDevice( const std::vector< Route >& routes ) {
    if( routes.empty() )
        return {};

    // NOTE: routes with higher priority are placed on the top of the routing table
    // NOTE: primary route is set manually and does not have 'metric' or 'proto'
    // Example: default via 192.168.89.1 dev eth0
    auto& route = routes.front();
    if( route.isDefault && route.metric.empty() && route.proto.empty() ) {
        return route.device;
    }

    return {};
}

std::vector< OpenwrtRoutesManager::Route > OpenwrtRoutesManager::readRoutes() {
    auto table = m_actionHandler->readRoutesTable();

    LOGGER_TRACE( cc::trace( "Read routes table: \n" ) + cc::note( table ) );

    std::vector< Route > routes;
    for( auto& row : StringUtils::splitByStrippedLinesWithText( table ) ) {
        routes.emplace_back( Route::parse( row ) );
    }
    return routes;
}

OpenwrtRoutesManager::Route OpenwrtRoutesManager::Route::parse( std::string route ) {
    OpenwrtRoutesManager::Route element;

    element.isDefault = route.find( "default" ) == 0;

    // TODO: rewrite to regular expressions when compiller will support them

    auto columns = StringUtils::split( route, " " );

    {
        auto it = std::find( columns.cbegin(), columns.cend(), "dev" );
        if( it != columns.cend() && ++it != columns.cend() )
            element.device = *it;
    }

    {
        auto it = std::find( columns.cbegin(), columns.cend(), "src" );
        if( it != columns.cend() && ++it != columns.cend() )
            element.source = *it;
    }

    {
        auto it = std::find( columns.cbegin(), columns.cend(), "via" );
        if( it != columns.cend() && ++it != columns.cend() )
            element.gateway = *it;
    }

    {
        auto it = std::find( columns.cbegin(), columns.cend(), "metric" );
        if( it != columns.cend() && ++it != columns.cend() )
            element.metric = *it;
    }

    {
        auto it = std::find( columns.cbegin(), columns.cend(), "proto" );
        if( it != columns.cend() && ++it != columns.cend() )
            element.proto = *it;
    }

    element.routeString = std::move( route );
    return element;
}

}  // namespace Network
}  // namespace HubPlatform
