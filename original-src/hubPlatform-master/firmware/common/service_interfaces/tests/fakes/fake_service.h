#pragma once

#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"

#include "database_client/fake_db/fake_db_client.h"

#include "test_utils/temporary_root_folder_wrapper.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Services {
namespace UT {

class FakeService : public Service {
public:
    FakeService()
    : Service( "test_service_name",
               std::make_unique< Services::BusWrapper >(
                   Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
               std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

    const FakeResponseBus& getFakeResponseBus() {
        return dynamic_cast< Services::UT::FakeResponseBus& >( *getBus() );
    }
};

// lm database is used
class FakeBridgeService : public Service {
public:
    FakeBridgeService()
    : Service( "test_service_name",
               std::make_unique< Services::BusWrapper >(
                   Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBridgeBus >{} ),
               nullptr ) {}

    const FakeResponseBridgeBus& getFakeResponseBus() {
        return dynamic_cast< Services::UT::FakeResponseBridgeBus& >( *getBus() );
    }

private:
    Tests::TemporaryRootFolderWrapper m_folderWrapper;
};

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
