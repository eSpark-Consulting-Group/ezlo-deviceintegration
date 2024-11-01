#include "2gig_reporter.h"
#include "json_utils/json_utils.h"

#include <unistd.h>

namespace HubPlatform {
namespace TwoGig {
TwoGigReporter::TwoGigReporter( const std::string& name,
                                std::shared_ptr< DevicesReachabilityManagerImpl >& reachabilityManager )
: Reporter( name )
, m_serviceName( name )
, m_reachabilityManager( reachabilityManager ) {}

std::string TwoGigReporter::prepareReportData() {
    std::string twoGigReportData = Reporter::prepareReportData();
    JsonObject twoGigReportJsonData;
    JsonObject twoGigNodeUpdateTimes;
    parseJson( twoGigReportData, twoGigReportJsonData );
    m_reachabilityManager->getDevicesUpdateTimes( twoGigNodeUpdateTimes );
    twoGigReportJsonData[ "service" ][ m_serviceName ][ "2gig_node_update_times" ] = twoGigNodeUpdateTimes;
    return twoGigReportJsonData.dump();
}
}  // namespace TwoGig
}  // namespace HubPlatform
