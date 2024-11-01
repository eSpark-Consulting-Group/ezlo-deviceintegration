#pragma once

#include "networkd/monitoring/nm_interface_monitor.h"
#include "networkd/resources/nm_resources.h"

#include "service_interfaces/task_loop.h"
#include "config_reader/config_reader.h"

namespace HubPlatform {
namespace Network {

class InterfaceMonitorFactory {
public:
    InterfaceMonitorFactory( Services::TaskLoop& taskLoop,
                             ConfigReader& configReader,
                             ExternalDataProvider& externalDataProvider );

    std::unique_ptr< InterfaceMonitor > createInterfaceMonitor(
        InterfaceInfoListener& interfaceInfoListener,
        std::string interfaceName,
        bool pingEnabled,
        std::chrono::seconds pingPeriod = std::chrono::seconds( Resources::PingPeriod ) ) const;

private:
    Services::TaskLoop& m_taskLoop;
    ExternalDataProvider& m_externalDataProvider;
    ConfigReader& m_configReader;
};

}  // namespace Network
}  // namespace HubPlatform
