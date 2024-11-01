#pragma once

#include "test_utils/component_tests/component_test_application.h"
#include "test_utils/component_tests/gateways_checker.h"

namespace HubPlatform {
namespace Tests {

class ScenesApiTestsApplication : public ComponentTestApplication {
public:
    ScenesApiTestsApplication();

protected:
    void beforeApplicationsStartup() override;
    void setUpAdditionalApplications() override;
    void afterApplicationsStartup() override;

private:
    std::unique_ptr< GatewaysChecker > m_gatewaysChecker;
};

}  // namespace Tests
}  // namespace HubPlatform
