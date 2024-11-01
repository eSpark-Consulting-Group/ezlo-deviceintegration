#pragma once

#include "service_interfaces/reporter.h"
#include <string>
#include <memory>

namespace HubPlatform {
namespace Metrics {

class CustomReporter : public Reporter {
public:
    explicit CustomReporter( const std::string& processName );

public:
    std::string prepareReportData() override;
    std::string getProcStatus() override;

    static pid_t fetchPidByProcessName( const std::string& serviceName );

private:
    std::string m_serviceName;
    pid_t m_pid{ 0 };
};

}  // namespace Metrics
}  // namespace HubPlatform