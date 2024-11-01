#pragma once

#include "cored/core_service.h"

#include "common_utils/cpp14.h"

#include "database_client/fake_db/fake_db_client.h"
#include "service_interfaces/bus_wrapper.h"
#include "bus/fake_bus.h"

namespace HubPlatform {
namespace CoreD {
namespace Tests {

class TestCoreService : public CoreService {
public:
    TestCoreService( std::unique_ptr< Services::BusWrapper > bus =
                         std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< FakeBus >{} ),
                     std::unique_ptr< IDbClient > dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >(),
                     ConfigReader&& configReader = ConfigReader() )
    : CoreService( "test", std::move( bus ), std::move( dbClient ), std::move( configReader ) ) {}
};

class TestCoreServiceWithoutHouseModes final : public TestCoreService {
public:
    TestCoreServiceWithoutHouseModes(
        std::unique_ptr< Services::BusWrapper > bus =
            std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< FakeBus >{} ),
        std::unique_ptr< IDbClient > dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >() )
    : TestCoreService( std::move( bus ), std::move( dbClient ) ) {}

    JsonObject getItem( const std::string& itemId ) {
        return CoreService::getItem( itemId );
    }

protected:
    void setUpHouseModes() override {}
    void setUpPredefinedItemGroups() override {}
    void setUpPredefinedDeviceGroups() override {}
    void setUpPredefinedContactIds() override {}
    void setUpPredefinedSecurityConfigs() override {}
    void setUpPredefinedPolicies() override {}
    void setUpGeneratedSensorGroups() override {}
};

}  // namespace Tests
}  // namespace CoreD
}  // namespace HubPlatform
