#include "zigbee_reporter.h"
#include "json_utils/json_utils.h"

#include <unistd.h>

namespace HubPlatform {
namespace Zigbee {
ZigbeeReporter::ZigbeeReporter( const std::string& name, std::shared_ptr< NodeManager >& nodeManager )
: Reporter( name )
, m_serviceName( name )
, m_nodeManagerPtr( nodeManager ) {}

std::string ZigbeeReporter::prepareReportData() {
    std::string zigbeeReportData = Reporter::prepareReportData();
    JsonObject zigbeeReportJsonData;
    JsonObject zigbeeNodeUpdateTimes;
    parseJson( zigbeeReportData, zigbeeReportJsonData );
    m_nodeManagerPtr->getZigbeeNodeUpdateTimes( zigbeeNodeUpdateTimes );
    zigbeeReportJsonData[ "service" ][ m_serviceName ][ "zigbee_node_update_times" ] = zigbeeNodeUpdateTimes;

    return zigbeeReportJsonData.dump();
}
}  // namespace Zigbee
}  // namespace HubPlatform
