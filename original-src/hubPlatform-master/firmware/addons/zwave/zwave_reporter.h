#pragma once

#include "service_interfaces/reporter.h"
#include "zwave/zwave_node_manager.h"
#include "zwave/zwave_node_statistics_collector.h"
#include <string>
#include <memory>
namespace HubPlatform {
namespace Zwave {
class ZwaveReporter : public Reporter {
public:
    explicit ZwaveReporter( const std::string& name,
                            std::shared_ptr< NodeManager >& nodeManager,
                            std::shared_ptr< NodeStatisticsCollector >& nodeStatistics );

private:
    std::string prepareReportData() override;

    Json extractCommandsStatistics();

    std::string m_serviceName;
    std::shared_ptr< NodeManager > m_nodeManagerPtr;
    std::shared_ptr< NodeStatisticsCollector > m_nodeStatistics;
};
}  // namespace Zwave
}  // namespace HubPlatform
