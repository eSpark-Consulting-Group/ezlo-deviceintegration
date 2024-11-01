#include "scenes_manager/tests/fakes/test_scenes_service.h"

#include "database_client/fake_db/fake_db_client.h"
#include "service_interfaces/bus_wrapper.h"
#include "bus/fake_bus.h"

namespace HubPlatform {
namespace ScenesManager {

TestService::TestService()
: TestService( "test_scene_service",
               std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< FakeBus >{} ),
               std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

TestService::TestService( const std::string& name,
                          std::unique_ptr< Services::BusWrapper > busWrapper,
                          std::unique_ptr< IDbClient > dbClient )
: ScenesManager::Service( name, std::move( busWrapper ), std::move( dbClient ) ) {}

void TestService::setUp() {
    ScenesManager::Service::setUp();
}

void TestService::tearDown() {
    ScenesManager::Service::tearDown();
}

}  // namespace ScenesManager
}  // namespace HubPlatform
