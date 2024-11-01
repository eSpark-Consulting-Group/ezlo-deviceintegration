#pragma once

#include "service_interfaces/reporter.h"
#include "gsm/nm_gsm_manager.h"
#include "networkd/network_manager.h"

namespace HubPlatform {
namespace Network {

class Service;

class NetworkReporter : public Reporter {
public:
    NetworkReporter( const std::string& name, Service& service, std::shared_ptr< NetworkManager > networkManager );
    ~NetworkReporter() override = default;
    std::string prepareReportData() override;

private:
    std::string m_serviceName;
    Service& m_networkService;
    std::shared_ptr< NetworkManager > m_networkManager;
};
}  // namespace Network
}  // namespace HubPlatform
