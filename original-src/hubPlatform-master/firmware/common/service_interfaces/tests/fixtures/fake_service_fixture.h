#pragma once

#include "service_interfaces/tests/fixtures/service_fixture.h"
#include "service_interfaces/tests/fakes/fake_service.h"

namespace HubPlatform {
namespace Services {
namespace UT {

class FakeServiceTestFixture : public ServiceFixture {
public:
    FakeService& getHostService() final {
        return m_fakeService;
    }

private:
    FakeService m_fakeService;
};

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
