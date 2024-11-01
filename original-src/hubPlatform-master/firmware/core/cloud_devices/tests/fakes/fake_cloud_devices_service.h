#pragma once

#include "service_interfaces/tests/fakes/fake_service.h"
#include "service_interfaces/tests/fixtures/fake_service_fixture.h"

namespace HubPlatform {
namespace UT {

class FakeCloudDevicesService : public Services::UT::ServiceFixture, public Services::UT::FakeService {
public:
    void SetUp() override {
        createDatabase();
        Services::UT::ServiceFixture::SetUp();
    }

    FakeService& getHostService() final override {
        return *this;
    }

    Services::UT::FakeResponseBus& getFakeBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( getBus() );
    }

    void initLogOptions() override{};
};

}  // namespace UT
}  // namespace HubPlatform
