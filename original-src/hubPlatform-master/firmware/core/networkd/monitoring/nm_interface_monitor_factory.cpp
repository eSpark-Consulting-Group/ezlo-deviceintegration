#include "networkd/monitoring/nm_interface_monitor_factory.h"
#include "networkd/monitoring/nm_dummy_interface_monitor.h"
#include "networkd/monitoring/nm_openwrt_interface_monitor.h"

namespace HubPlatform {
namespace Network {

InterfaceMonitorFactory::InterfaceMonitorFactory( Services::TaskLoop& taskLoop,
                                                  ConfigReader& configReader,
                                                  ExternalDataProvider& externalDataProvider )
: m_taskLoop( taskLoop )
, m_configReader( configReader )
, m_externalDataProvider( externalDataProvider ) {}

std::unique_ptr< InterfaceMonitor > InterfaceMonitorFactory::createInterfaceMonitor(
    InterfaceInfoListener& interfaceInfoListener,
    std::string interfaceName,
    bool pingEnabled,
    std::chrono::seconds pingPeriod ) const {
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    return std::make_unique< DummyInterfaceMonitor >(
        m_taskLoop, interfaceInfoListener, std::move( interfaceName ), m_configReader, m_externalDataProvider );
#else
    return std::make_unique< OpenwrtInterfaceMonitor >( m_taskLoop,
                                                        interfaceInfoListener,
                                                        std::move( interfaceName ),
                                                        m_configReader,
                                                        m_externalDataProvider,
                                                        pingEnabled,
                                                        pingPeriod );
#endif
}

}  // namespace Network
}  // namespace HubPlatform
