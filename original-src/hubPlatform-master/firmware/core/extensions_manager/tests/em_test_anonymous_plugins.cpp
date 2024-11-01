#include "extensions_manager/common/anonymous_plugin_utils.h"
#include "extensions_manager/resources/em_resources.h"

#include "common_utils/cpp14.h"
#include "bus/fake_bus.h"
#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbid.h"
#include "database_client/dbtools.h"

#include <gtest/gtest.h>

#include <memory>

/*

TESTPLAN

DONE    1. check anonymous plugin creation
DONE        1.1 check anonymous plugin creation
DONE        1.2 check anonymous plugin creation with no code
DONE        1.3 check anonymous plugin creation with too long code
DONE        1.4 check anonymous plugin creation with too long name
DONE    2. check anonymous plugin changing
DONE        2.1 check anonymous plugin changing
DONE        2.2 check anonymous plugin changing with invalid plugin id
DONE        2.3 check anonymous plugin changing with invalid name
DONE        2.4 check anonymous plugin changing with invalid code
DONE        2.5 check anonymous plugin changing without changes
DONE    3. check anonymous plugin deleting
DONE        3.1 check anonymous plugin deleting
DONE        3.2 check anonymous plugin deleting with invalid plugin id
DONE        3.3 check anonymous plugin deleting of acquired plugin
DONE    4. check anonymous plugin run
DONE        4.1 check anonymous plugin run
DONE        4.2 check anonymous plugin run with invalid plugin id
DONE    5. check is anonymous plugin acquired
DONE        5.1 check is anonymous plugin acquired
DONE    6. check anonymous plugin acquiring
DONE        6.1 check anonymous plugin acquiring
DONE        6.2 check anonymous plugin acquiring with invalid plugin id
DONE    7. check anonymous plugin releasing
DONE        7.1 check anonymous plugin releasing
DONE        7.2 check anonymous plugin releasing with invalid plugin id
DONE    8. check anonymous plugin existence
DONE        8.1 check anonymous plugin existence

*/

namespace HubPlatform {
namespace UT {

using namespace ExtensionsManager::Resources;

class FakeService final : public Services::Service {
public:
    FakeService()
    : Services::Service( "",
                         std::make_unique< Services::BusWrapper >(
                             Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
                         std::make_unique< FakeDbClient >() ) {}

    void setUp() {
        Services::Service::setUp();
    }

    void tearDown() {
        Services::Service::tearDown();
    }

    BaseBus* getBus() {
        return Services::Service::getBus();
    }
};

class AnonymousPluginUtilsSuite : public testing::Test {
public:
    AnonymousPluginUtilsSuite()
    : m_anonymousPluginUtils( m_service ) {}

    void SetUp() override {
        m_service.setUp();
        m_service.getDBClient().clean();
    }

    void TearDown() override {
        m_service.tearDown();
    }

    ExtensionsManager::AnonymousPluginUtils& getAnonymousPluginUtils() {
        return m_anonymousPluginUtils;
    }

    const ExtensionsManager::AnonymousPluginUtils& getAnonymousPluginUtils() const {
        return m_anonymousPluginUtils;
    }

    IDbClient& getDbClient() {
        return m_service.getDBClient();
    }

    const IDbClient& getDbClient() const {
        return m_service.getDBClient();
    }

    const Services::UT::FakeResponseBus& getTestBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( m_service.getBus() );
    }

private:
    FakeService m_service;
    ExtensionsManager::AnonymousPluginUtils m_anonymousPluginUtils;
};

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginCreation ) { /*1.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_TRUE( getDbClient().has( DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ) ) );
    EXPECT_TRUE( getDbClient().exist( getPluginDBName( pluginId ) ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginCreationFailNoCode ) { /*1.2*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "", "the best plugin ever" );

    EXPECT_TRUE( pluginId.empty() );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginCreationFailTooLongCode ) { /*1.3*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    std::string longCode( AnonymousPlugin::AnonymousPluginContentMaxSize, 'a' );
    const auto pluginId = anonymousPluginUtils.addPlugin( longCode, "the best plugin ever" );

    EXPECT_TRUE( pluginId.empty() );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginCreationFailTooLongName ) { /*1.4*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    std::string longName( AnonymousPlugin::AnonymousPluginNameMaxSize, 'a' );
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", longName );

    EXPECT_TRUE( pluginId.empty() );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginChanging ) { /*2.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_TRUE( anonymousPluginUtils.changePlugin( pluginId, "bye", "the worst plugin ever" ) );

    const auto plugin =
        getDbClient().findOne( DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );
    const auto code = json_get_str( plugin, AnonymousPlugin::Fields::Code );
    const auto name = json_get_str( plugin, AnonymousPlugin::Fields::Name );

    EXPECT_EQ( code, "bye" );
    EXPECT_EQ( name, "the worst plugin ever" );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginChangingFailInvalidPluginId ) { /*2.2*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_FALSE( anonymousPluginUtils.changePlugin( "invalid_plugin_id", "wassup", "the worst plugin ever" ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginChangingFailInvalidName ) { /*2.3*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );
    std::string longName( AnonymousPlugin::AnonymousPluginNameMaxSize, 'a' );

    EXPECT_FALSE( anonymousPluginUtils.changePlugin( pluginId, "", longName ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginChangingFailInvalidCode ) { /*2.4*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );
    std::string longCode( AnonymousPlugin::AnonymousPluginContentMaxSize, 'a' );

    EXPECT_FALSE( anonymousPluginUtils.changePlugin( pluginId, longCode, "" ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginChangingFailNothingToChange ) { /*2.5*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_FALSE( anonymousPluginUtils.changePlugin( pluginId, "", "" ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginDelete ) { /*3.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_TRUE( anonymousPluginUtils.deletePlugin( pluginId ) );
    EXPECT_FALSE( getDbClient().has( DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ) ) );
    EXPECT_FALSE( getDbClient().exist( getPluginDBName( pluginId ) ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginDeleteFailInvalidPluginId ) { /*3.2*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_FALSE( anonymousPluginUtils.deletePlugin( "invalid_plugin_id" ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginDeleteFailPluginAcquired ) { /*3.3*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    anonymousPluginUtils.acquirePlugin( pluginId, true );
    const auto result = anonymousPluginUtils.deletePlugin( pluginId );

    EXPECT_EQ( result.getError(), Errors::ANONYMOUS_PLUGIN_ACQUIRED );
    EXPECT_FALSE( result );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginRun ) { /*4.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    const auto result = anonymousPluginUtils.runPlugin( pluginId );
    EXPECT_EQ( result.getError(), Errors::LUA_SCRIPT_ERROR );
    EXPECT_FALSE( result );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginRunFailInvalidPluginId ) { /*4.2*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_FALSE( anonymousPluginUtils.runPlugin( "invalid_plugin_id" ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkIsAnonymousPluginAcquired ) { /*5.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_FALSE( anonymousPluginUtils.isPluginAcquired( "invalid_plugin_id" ) );
    EXPECT_FALSE( anonymousPluginUtils.isPluginAcquired( pluginId ) );

    anonymousPluginUtils.acquirePlugin( pluginId, true );
    EXPECT_TRUE( anonymousPluginUtils.isPluginAcquired( pluginId ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginAcquire ) { /*6.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_TRUE( anonymousPluginUtils.acquirePlugin( pluginId, true ) );
    const auto result = anonymousPluginUtils.deletePlugin( pluginId );

    EXPECT_EQ( result.getError(), Errors::ANONYMOUS_PLUGIN_ACQUIRED );
    EXPECT_FALSE( result );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginAcquireFailInvalidPluginId ) { /*6.2*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_FALSE( anonymousPluginUtils.acquirePlugin( "invalid_plugin_id" ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginRelease ) { /*7.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    anonymousPluginUtils.acquirePlugin( pluginId );
    anonymousPluginUtils.releasePlugin( pluginId );

    EXPECT_TRUE( getDbClient().has( DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ) ) );
    EXPECT_TRUE( getDbClient().exist( getPluginDBName( pluginId ) ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginReleaseFailInvalidPluginId ) { /*7.2*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    anonymousPluginUtils.acquirePlugin( pluginId );
    anonymousPluginUtils.releasePlugin( "invalid_plugin_id" );

    EXPECT_TRUE( getDbClient().has( DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ) ) );
    EXPECT_TRUE( getDbClient().exist( getPluginDBName( pluginId ) ) );
}

TEST_F( AnonymousPluginUtilsSuite, checkAnonymousPluginExist ) { /*8.1*/
    auto& anonymousPluginUtils = getAnonymousPluginUtils();
    const auto pluginId = anonymousPluginUtils.addPlugin( "wassup", "the best plugin ever" );

    EXPECT_TRUE( anonymousPluginUtils.pluginExists( pluginId ) );

    anonymousPluginUtils.deletePlugin( pluginId );
    EXPECT_FALSE( anonymousPluginUtils.pluginExists( pluginId ) );
    EXPECT_FALSE( anonymousPluginUtils.pluginExists( "invalid_plugin_id" ) );
}

}  // namespace UT
}  // namespace HubPlatform
