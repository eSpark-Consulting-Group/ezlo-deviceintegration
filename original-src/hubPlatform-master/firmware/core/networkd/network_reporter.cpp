#include "networkd/network_reporter.h"
#include "networkd/network_service.h"

#include "json_utils/json_utils.h"
#include <utility>

namespace HubPlatform {
namespace Network {
NetworkReporter::NetworkReporter( const std::string& name,
                                  Service& service,
                                  std::shared_ptr< NetworkManager > networkManager )
: Reporter( name )
, m_serviceName( name )
, m_networkService( service )
, m_networkManager( std::move( networkManager ) ) {}

std::string NetworkReporter::prepareReportData() {
    JsonObject networkReportJsonData;
    JsonObject serviceData;

    std::string networkReportData = Reporter::prepareReportData();
    parseJson( networkReportData, networkReportJsonData );

    if( json_get( networkReportJsonData, { "service", m_serviceName }, serviceData ) ) {
        serviceData[ "modems" ] = m_networkManager->getGSMManager().getModemInfo();
        serviceData[ "ui_network_configuration" ] = m_networkManager->getUINetworkConfiguration();
        serviceData[ "active_interface" ] = m_networkManager->getActiveInterface();
        serviceData[ "wifi_availability" ] = m_networkManager->isWifiInternetAvailable();
        serviceData[ "internet_availability" ] = m_networkManager->isInternetAvailable();
        serviceData[ "non_mobile_internet_availability" ] = m_networkManager->isNonMobileInternetAvailable();
        serviceData[ "active_wifi_device" ] = m_networkManager->getActiveWifiDevice();
        serviceData[ "ssh_tunnels" ] = m_networkService.dumpSshTunnelsMetricsData();
        serviceData[ "ping_statistics" ] = m_networkManager->getPingStatisticsJson();
    }

    networkReportJsonData[ "service" ][ m_serviceName ] = serviceData;

    return networkReportJsonData.dump();
}

}  // namespace Network
}  // namespace HubPlatform
