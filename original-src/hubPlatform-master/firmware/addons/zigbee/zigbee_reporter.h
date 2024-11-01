#pragma once

#include "service_interfaces/reporter.h"
#include "zigbee/zigbee_node_manager.h"
#include <string>
#include <memory>
namespace HubPlatform {
namespace Zigbee {
class ZigbeeReporter : public Reporter {
public:
    explicit ZigbeeReporter( const std::string& name, std::shared_ptr< NodeManager >& nodeManager );

private:
    std::string prepareReportData() override;
    std::string m_serviceName;
    std::shared_ptr< NodeManager > m_nodeManagerPtr;
};
}  // namespace Zigbee
}  // namespace HubPlatform
