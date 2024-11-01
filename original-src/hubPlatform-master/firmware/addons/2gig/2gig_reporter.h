#pragma once

#include "service_interfaces/reporter.h"
#include "2gig/2gig_devices_reachability_manager.h"

#include <string>
#include <memory>

namespace HubPlatform {
namespace TwoGig {
class TwoGigReporter : public Reporter {
public:
    explicit TwoGigReporter( const std::string& name,
                             std::shared_ptr< DevicesReachabilityManagerImpl >& reachabilityManager );
    std::string prepareReportData() override;

private:
    std::string m_serviceName;
    std::shared_ptr< DevicesReachabilityManagerImpl > m_reachabilityManager;
};
}  // namespace TwoGig
}  // namespace HubPlatform
