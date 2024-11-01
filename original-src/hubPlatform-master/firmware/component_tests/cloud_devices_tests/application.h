#pragma once

#include "test_utils/component_tests/component_test_application.h"
#include "test_utils/component_tests/gateways_checker.h"
#include "system_utils/process/forker.h"

namespace HubPlatform {
namespace Tests {

class CloudDevicesTestsApplication : public ComponentTestApplication {
public:
    CloudDevicesTestsApplication();
    int getCustomLocalCloudDataWebServerPort() const;

protected:
    void beforeApplicationsStartup() override;
    void setUpAdditionalApplications() override;
    void afterApplicationsStartup() override;

    bool shouldUseRandomLocalPort() override;
    void tearDown() override;

private:
    void setUpCustomLocalCloudDataWebServer();

private:
    std::mutex m_requestsMutex;
    std::condition_variable m_requestsCondVar;
    std::deque< JsonObject > m_requestsMessages;
    std::unique_ptr< GatewaysChecker > m_gatewaysChecker;
    Process::Forker::Instance m_forker;
    pid_t m_customLocalCloudDataWebServerPid = -1;
    int m_customLocalCloudDataWebServerPort = -1;
};

}  // namespace Tests
}  // namespace HubPlatform
