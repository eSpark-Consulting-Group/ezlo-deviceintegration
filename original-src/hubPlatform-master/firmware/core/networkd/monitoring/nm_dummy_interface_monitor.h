#pragma once

#include "networkd/monitoring/nm_interface_monitor.h"
#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/monitoring/nm_ping_statistics.h"
#include "networkd/routes/nm_dummy_routes_manager.h"
#include "networkd/utils/nm_utils.h"

#include "service_interfaces/task_loop.h"
#include "service_interfaces/task_loop_timer.h"
#include "system_utils/system.h"

#include "json_utils/json_utils.h"

#include "tiny/include/tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

#include <algorithm>

namespace HubPlatform {
namespace Network {

class DummyInterfaceMonitor : public InterfaceMonitor {
public:
    DummyInterfaceMonitor( Services::TaskLoop& taskLoop,
                           InterfaceInfoListener& interfaceInfoListener,
                           std::string interfaceName,
                           ConfigReader& configReader,
                           ExternalDataProvider& externalDataProvider )
    : InterfaceMonitor( taskLoop,
                        interfaceInfoListener,
                        std::move( interfaceName ),
                        configReader,
                        externalDataProvider,
                        false,
                        std::chrono::seconds( 0 ) )
    , m_info( JSON( "_id" << m_interfaceName << "status"
                          << "down"
                          << "internetAvailable" << false ) )
    , m_active( false ) {}

    void start() override {
        m_active = true;
        m_interfaceChangeListener.onNewInterfaceInfo( getInfo() );
    };

    void stop() override {
        m_active = false;
    };

    bool isActive() override {
        return m_active;
    };

    const Json& manualUpdate() override {
        return getInfo();
    };

    const Json& getInfo() override {
        using namespace Utils;

        if( m_interfaceName == Resources::EthernetInterfaceName ) {
            static Json info = m_info;

            auto routeDevice = DummyRoutesManager().getDefaultRouteDevice();
            auto interfaces = Utils::getSystemInterfaces();

            if( !routeDevice.empty() ) {
                info[ "status" ] = "up";
                info[ "internetAvailable" ] = true;

                for( const auto& interface : interfaces ) {
                    if( interface.m_name == routeDevice ) {
                        auto ipv4It = std::find_if( interface.m_addressList.begin(),
                                                    interface.m_addressList.end(),
                                                    []( const InterfaceInfo::Address& addr ) {
                                                        return addr.m_type == InterfaceInfo::Type::IPV4;
                                                    } );
                        if( ipv4It == interface.m_addressList.end() )
                            continue;

                        info[ "ipv4" ][ "ip" ] = Utils::sockadrToString( ipv4It->m_ipAddress );
                        info[ "ipv4" ][ "mask" ] = Utils::sockadrToString( ipv4It->m_mask );
                        std::string gateway = "error";
                        try {
                            gateway = System::getAwkOutput( "route -n | grep 'UG[ \t]' | awk '{print $2}'" );
                        }
                        catch( const System::PopenFailed& ex ) {
                            LOGGER_ERROR( cc::error( "Failed to parse routes: " ) + ex.what() );
                        }
                        info[ "ipv4" ][ "gateway" ] = gateway;
                        info[ "ipv4" ][ "mode" ] = "dhcp";
                    }
                }
            }
            else {
                info[ "status" ] = "down";
                info[ "internetAvailable" ] = false;
                info[ "ipv4" ] = JSON_EMPTY_OBJ;
                info[ "ipv4" ][ "mode" ] = "dhcp";
            }

            if( m_info != info ) {
                m_info = info;
                m_interfaceChangeListener.onNewInterfaceInfo( m_info );
            }
        }

        return m_info;
    };

    bool isInternetAvailable() override {
        return true;
    };
    bool configure() override {
        return true;
    }

    const PingStatistics& pingStatistics() override {
        return m_pingStatistics;
    }

private:
    PingStatistics m_pingStatistics;
    Json m_info;
    bool m_active;
};

}  // namespace Network
}  // namespace HubPlatform
