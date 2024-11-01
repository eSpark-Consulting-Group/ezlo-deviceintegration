#pragma once

#include "test_utils/component_tests/component_test_application.h"
#include "test_utils/component_tests/gateways_checker.h"

#include "system_utils/process/forker.h"

namespace HubPlatform {
namespace Tests {

class NmaApiTestsApplication : public ComponentTestApplication {
public:
    NmaApiTestsApplication();

    int getCustomPluginsWebServerPort() const;
    RequiredGateways getRequiredGateways() const override;

protected:
    void beforeApplicationsStartup() override;
    void setUpAdditionalApplications() override;
    void afterApplicationsStartup() override;
    bool shouldUseRandomLocalPort() override;
    void tearDown() override;

private:
    void setUpCustomPluginsWebServer();

private:
    std::unique_ptr< GatewaysChecker > m_gatewaysChecker;
    Process::Forker::Instance m_forker;
    pid_t m_customPluginsWebServerPid = -1;
    int m_customPluginsWebServerPort = -1;
};

}  // namespace Tests
}  // namespace HubPlatform
