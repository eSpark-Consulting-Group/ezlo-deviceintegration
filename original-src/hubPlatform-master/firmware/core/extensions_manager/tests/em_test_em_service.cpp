#include "extensions_manager/em_service.h"

#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/message.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "bus/messagesclasses.h"
#include "bus/fake_bus.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "database_client/fake_db/fake_db_client.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

using namespace ::testing;

class EmServiceTestSuite : public Test {
protected:
    EmServiceTestSuite()
    : m_emd( "test",
             std::make_unique< Services::BusWrapper >(
                 Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
             std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

    void SetUp() override {
        m_emd.Services::Service::setUp();
        cleanDB();
    }

    void TearDown() override {
        cleanDB();
        m_emd.Services::Service::tearDown();
    }

    ExtensionsManager::Service& getService() {
        return m_emd;
    }

    IDbClient& getDBClient() {
        return getService().getDBClient();
    }

    void cleanDB() {
        getDBClient().clean();
    }

    const Services::UT::FakeResponseBus& getTestBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( getService().getBus() );
    }

protected:
    void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) {
        m_emd.sendExecutePluginScript( script, scriptParams, "", false, 1 );
    }

    void sendExecutePluginScript( const std::string& script ) {
        sendExecutePluginScript( script, JSON_EMPTY_OBJ );
    }

private:
    ExtensionsManager::Service m_emd;
};

TEST_F( EmServiceTestSuite, sendExecutePluginScript ) {
    const auto scriptName = "HUB:test/hello_world";

    sendExecutePluginScript( scriptName );

    auto message = getTestBus().getMessage();
    EXPECT_EQ( message.group, "addon.lua" );
    EXPECT_EQ( message.msgClass, REQUEST );
    EXPECT_EQ( message.msgSubClass, EXECUTE_SCRIPT );

    auto data = JSON_EMPTY_OBJ;
    data[ "script" ] = scriptName;
    data[ "isUserRunScript" ] = false;
    data[ "operationId" ] = "";

    data.erase( "id" );
    message.data.erase( "id" );
    EXPECT_EQ( message.data[ "requestBody" ].dump(), data.dump() );
}

}  // namespace UT
}  // namespace HubPlatform
