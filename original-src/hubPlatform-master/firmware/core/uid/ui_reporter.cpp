#include "ui_reporter.h"
#include "http_bridge/ws_api_bridge.h"

namespace HubPlatform {
namespace Uid {

UIReporter::UIReporter( const std::string& name,
                        const EnvironmentPtr& environmentPtr,
                        const NmaClientPtr& nmaClientPtr,
                        ServiceSettings& settings,
                        ConnectionNmaReporter& connectionReporter )
: Reporter( name )
, m_serviceName( name )
, m_environmentPtr( environmentPtr )
, m_nmaClientPtr( nmaClientPtr )
, m_serviceSettings( settings )
, m_connectionReporter( connectionReporter ) {}

std::string UIReporter::prepareReportData() {
    JsonObject uiReportJsonData, serviceData;
    std::string uiReportData = Reporter::prepareReportData();

    if( parseJson( uiReportData, uiReportJsonData ) ) {
        try {
            if( json_get( uiReportJsonData, { "service", m_serviceName.c_str() }, serviceData ) ) {
                serviceData[ "nma_connection" ] = Ezlo::isConnected();
                serviceData[ "zwave_calibration" ] = Ezlo::getZwaveCalibration();
                serviceData[ "hub_errors" ] = Ezlo::getHubErrors();

                if( m_environmentPtr ) {
                    serviceData[ "cloud_coordinator_url" ] = m_environmentPtr->getCloudCoordinatorURL();
                    serviceData[ "explicit_cloud_coordinator_url" ] =
                        m_environmentPtr->getExplicitCloudCoordinatorURL();
                    serviceData[ "nma_server_url" ] = m_environmentPtr->getNmaServerURL( false );
                }

                serviceData[ "bus_connection_status" ] = !m_serviceSettings.isNoBus();
                m_connectionReporter.updateNmaConnectionReport();
                serviceData[ "Nma" ] = m_connectionReporter.updateAndGetCurrentNmaConnectionReport();

                auto httpMetricsData = HttpBridge::getApiBridge().getMetrics();
                serviceData[ "HttpServer" ] = httpMetricsData;

                if( m_nmaClientPtr ) {
                    serviceData[ "hub_registration_status" ] = m_nmaClientPtr->isHubRegistered();
                    serviceData[ "hub_connecting_status" ] = m_nmaClientPtr->isConnecting();
                }

                uiReportJsonData[ "service" ][ m_serviceName ] = serviceData;
            }

            return uiReportJsonData.dump();
        }
        catch( const std::exception& ex ) {
            const char* strWhat = ex.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown exception";
            Log::error( cc::error( "Top level, caught exception:" ) + cc::warn( strWhat ) );
        }
        catch( ... ) {
            Log::error( cc::error( "Top level, caught unknown exception." ) );
        }
    }

    return uiReportData;
}

}  // namespace Uid
}  // namespace HubPlatform
