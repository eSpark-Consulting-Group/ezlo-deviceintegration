#include "cored/walk_test/walk_test.h"
#include "test_utils/test_utils.h"
#include "cored/tests/service/test_core_service.h"
#include <memory>
#include "uid/response_builders/hub_modes_get/response_builder_v3.h"
#include "config_reader/tests/config_reader_factory.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/model/resources/cored_resources.h"
#include "base_utils/timeutils.h"

// Test plan:

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace HubPlatform::CoreD;
using namespace Services;
using namespace CoreD::Model::Resources;
using namespace CoreD::WalkTest;
const int TestRequireExtendedPeriod = 1;
const int TestDurationPeriod = 2;
const int TestExtendPeriod = 1;
const char* const OperationId = "operationId";
const char* const Type = "type";
const char* const Completed = "completed";
const char* const ProtectNotification = "protectNotification";
const char* const Status = "status";
class WalkTestSuite : public ::testing::Test, public ConfigReaderFactory {
public:
    std::unique_ptr< HubPlatform::CoreD::WalkTest::WalkTest >& getWalkTest();
    ConfigReader getConfig();
    void SetUp() override;
    Service& getService();
    Services::UT::FakeResponseBus& getFakeResponseBus();

public:
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
    MessageSubClasses m_msgClass = UNKNOWN;
};

Service& WalkTestSuite::getService() {
    return *m_service;
}

void WalkTestSuite::SetUp() {
    m_service =
        std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >( std::make_unique< Services::BusWrapper >(
            Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >() ) );

    m_service->setUp();
}

Services::UT::FakeResponseBus& WalkTestSuite::getFakeResponseBus() {
    return *dynamic_cast< Services::UT::FakeResponseBus* >( m_service->getBus() );
}

ConfigReader WalkTestSuite::getConfig() {
    JsonObject configData = JSON( "settings" << JSON( "period" << TestRequireExtendedPeriod ) );

    std::string stringData = configData.dump();
    return ConfigReaderFactory::createConfigReader( [stringData]( const std::string& ) { return stringData; } );
}

TEST_F( WalkTestSuite, getConfigPeriod ) {
    auto periodFromConfig = CoreD::WalkTest::WalkTest::getConfigExtendPeriod( getConfig() );
    EXPECT_EQ( periodFromConfig.count(), TestRequireExtendedPeriod );
}

TEST_F( WalkTestSuite, testWithExtend ) {
    std::chrono::seconds TestDuration = std::chrono::seconds( 2 );
    auto periodFromConfig = CoreD::WalkTest::WalkTest::getConfigExtendPeriod( getConfig() );
    EXPECT_EQ( periodFromConfig.count(), TestRequireExtendedPeriod );

    auto walkTest = std::make_unique< HubPlatform::CoreD::WalkTest::WalkTest >( getService(), getConfig() );
    walkTest->setDurationSeconds( std::chrono::seconds( TestDurationPeriod ) );
    walkTest->setExtendSeconds( std::chrono::seconds( TestExtendPeriod ) );

    Json params = JSON( OperationId << "operationId" << Type << "walk" << ProtectNotification << true );
    walkTest->protectSystemTestStart( params );
    EXPECT_EQ( walkTest->getDuration().count(), 2 );
    auto message = getFakeResponseBus().getMessage();
    EXPECT_FALSE( message.group.empty() );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, WALK_TEST_PROGRESS );
    EXPECT_FALSE( message.data.empty() );
    auto data = message.data;
    EXPECT_EQ( data[ Completed ], 0 );
    EXPECT_EQ( data[ OperationId ], OperationId );
    EXPECT_EQ( data[ Status ], "started" );
    sleep( TestRequireExtendedPeriod );
    walkTest->protectSystemTestExtend( params );
    EXPECT_FALSE( message.group.empty() );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, WALK_TEST_PROGRESS );
    EXPECT_FALSE( message.data.empty() );
    message = getFakeResponseBus().getMessage();
    data = message.data;
    EXPECT_EQ( data[ Completed ], 80 );
    EXPECT_EQ( data[ OperationId ], OperationId );
    EXPECT_EQ( data[ Status ], "extended" );
    EXPECT_EQ( walkTest->getDuration().count(), 3 );
}

}  // namespace UT
}  // namespace HubPlatform
