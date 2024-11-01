#include "zwave_reporter.h"
#include "json_utils/json_utils.h"

#include <unistd.h>

namespace HubPlatform {
namespace Zwave {

ZwaveReporter::ZwaveReporter( const std::string& name,
                              std::shared_ptr< NodeManager >& nodeManager,
                              std::shared_ptr< NodeStatisticsCollector >& nodeStatistics )
: Reporter( name )
, m_serviceName( name )
, m_nodeManagerPtr( nodeManager )
, m_nodeStatistics( nodeStatistics ) {}

std::string ZwaveReporter::prepareReportData() {
    std::string zwaveReportData = Reporter::prepareReportData();
    JsonObject zwaveReportJsonData;
    JsonObject zwaveNodeUpdateTimes;
    parseJson( zwaveReportData, zwaveReportJsonData );
    m_nodeManagerPtr->getZwaveNodeUpdateTimes( zwaveNodeUpdateTimes );
    zwaveReportJsonData[ "service" ][ m_serviceName ][ "zwave_node_update_times" ] = zwaveNodeUpdateTimes;
    zwaveReportJsonData[ "service" ][ m_serviceName ][ "always_on_devices_interval" ] =
        m_nodeManagerPtr->getAlwaysOnDevicesInterval();
    zwaveReportJsonData[ "service" ][ m_serviceName ][ "zwave_commands_statistics" ] = extractCommandsStatistics();

    return zwaveReportJsonData.dump();
}

Json ZwaveReporter::extractCommandsStatistics() {
    JsonArray completedCommands;
    JsonArray failedCommands;

    m_nodeStatistics->fetchStatisctics( [&completedCommands,
                                         &failedCommands]( const NodeStatisticsCollector::CommandInfo& info ) {
        auto jsonInfo = JSON(
            "node_id"
            << info.m_nodeId << "when"
            << std::chrono::duration_cast< std::chrono::seconds >( info.m_commandSent.time_since_epoch() ).count() );

        switch( info.m_status ) {
        case NodeStatisticsCollector::CommandInfo::Status::Sent:
            jsonInfo[ "delay" ] = std::chrono::duration_cast< std::chrono::milliseconds >( info.m_delay ).count();
            completedCommands.push_back( std::move( jsonInfo ) );
            break;
        case NodeStatisticsCollector::CommandInfo::Status::Failed:
            failedCommands.push_back( std::move( jsonInfo ) );
            break;
        default:
            LOGGER_DEBUG( cc::debug( "Incorrect CommandInfo Status" ) );
        }
    } );

    return JSON( "completed_commands" << std::move( completedCommands ) << "failed_commands"
                                      << std::move( failedCommands ) );
}

}  // namespace Zwave
}  // namespace HubPlatform
