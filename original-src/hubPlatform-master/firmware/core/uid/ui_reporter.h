#pragma once

#include "service_interfaces/reporter.h"
#include "uid/environment.h"
#include "uid/nma_client.h"
#include "uid/service_settings.h"
#include "reporter/connection_nma_reporter.h"
#include "uid/ezlo.h"

namespace HubPlatform {
namespace Uid {
class UIReporter : public Reporter {
public:
    explicit UIReporter( const std::string& name,
                         const EnvironmentPtr& environmentPtr,
                         const NmaClientPtr& nmaClientPtr,
                         ServiceSettings& settings,
                         ConnectionNmaReporter& connectionReporter );

    ~UIReporter() override = default;

    std::string prepareReportData() override;

private:
    std::string m_serviceName;
    const EnvironmentPtr& m_environmentPtr;
    const NmaClientPtr& m_nmaClientPtr;
    ServiceSettings& m_serviceSettings;
    ConnectionNmaReporter& m_connectionReporter;
};
}  // namespace Uid
}  // namespace HubPlatform
