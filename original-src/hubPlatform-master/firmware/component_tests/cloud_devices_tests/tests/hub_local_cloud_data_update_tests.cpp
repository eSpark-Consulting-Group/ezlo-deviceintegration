#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cloud_devices_tests/application.h"
#include "config_reader/config_reader.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class HubLocalCloudDataUpdateTest : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

    static std::string makeWebUrl( const std::string& archiveName );
    static int getWebServerPort();
    void checkProgressBroadcast( const std::string& status,
                                 int completed,
                                 const std::string& action,
                                 const std::string& source );
};
void HubLocalCloudDataUpdateTest::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void HubLocalCloudDataUpdateTest::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

std::string HubLocalCloudDataUpdateTest::makeWebUrl( const std::string& archiveName ) {
    return "http://localhost:" + std::to_string( getWebServerPort() ) + "/" + archiveName;
}

int HubLocalCloudDataUpdateTest::getWebServerPort() {
    return dynamic_cast< CloudDevicesTestsApplication& >( nma() ).getCustomLocalCloudDataWebServerPort();
}

void HubLocalCloudDataUpdateTest::checkProgressBroadcast( const std::string& status,
                                                          int completed,
                                                          const std::string& action,
                                                          const std::string& source ) {
    auto broadcast = waitForBroadcast( "hub.offline.cloud.data.update.progress", "progress broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.offline.cloud.data.update.progress" );
    JsonObject result;
    json_get_obj( broadcast, "result", result );
    EXPECT_EQ( json_get_str( result, "action" ), action );
    EXPECT_EQ( json_get_str( result, "status" ), status );
    EXPECT_EQ( json_get_int( result, "completed" ), completed );
    EXPECT_EQ( json_get_str( result, "source" ), source );
}

TEST_F( HubLocalCloudDataUpdateTest, hubLocalCloudDataUpdateSuccess ) {
    JsonObject error;
    const std::string url = makeWebUrl( "tcds_240808144411.tgz" );
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.offline.cloud.data.update" },
                                                       { "params", JSON( "current_version" << 2 << "url" << url ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );
}

TEST_F( HubLocalCloudDataUpdateTest, hubLocalCloudDataUpdateFail ) {
    JsonObject error;
    {
        const std::string url = makeWebUrl( "tcds_240808144411.tgz" );
        const auto answer =
            nma().sendtoHubAndGetAnswer( { { "method", "hub.offline.cloud.data.update" },
                                           { "params", JSON( "current_version" << -1 << "url" << url ) } } );

        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
        EXPECT_EQ( json_get_str( error, "reason" ), "current_version is not updated one" );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.offline.cloud.data.update" },
                                                           { "params",
                                                             JSON( "current_version" << 3 << "url"
                                                                                     << "" ) } } );

        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
        EXPECT_EQ( json_get_str( error, "reason" ), "url is empty" );
    }
}

TEST_F( HubLocalCloudDataUpdateTest, hubLocalCloudDataUpdateSuccessMeshbots ) {
    JsonObject error;
    const std::string url = makeWebUrl( "tcds_240808144411.tgz" );
    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.offline.cloud.data.update" },
          { "params", JSON( "meshbots_current_version" << 2 << "meshbots_url" << url ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );
}

TEST_F( HubLocalCloudDataUpdateTest, hubLocalCloudDataUpdateFailMeshbots ) {
    JsonObject error;
    {
        const std::string url = makeWebUrl( "tcds_240808144411.tgz" );
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.offline.cloud.data.update" },
              { "params", JSON( "meshbots_current_version" << -1 << "meshbots_url" << url ) } } );

        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
        EXPECT_EQ( json_get_str( error, "reason" ), "meshbots_current_version is not updated one" );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.offline.cloud.data.update" },
                                                           { "params",
                                                             JSON( "meshbots_current_version" << 3 << "meshbots_url"
                                                                                              << "" ) } } );

        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
        EXPECT_EQ( json_get_str( error, "reason" ), "meshbots_url is empty" );
    }
}

TEST_F( HubLocalCloudDataUpdateTest, hubOfflineCloudDataInfoGet ) {
    JsonObject error, result, cloudData, meshbotTemplates;

    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.offline.cloud.data.info.get" }, { "params", {} } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );
    ASSERT_TRUE( json_get_obj( answer, "result", result ) );
    ASSERT_TRUE( json_get_obj( result, "cloudData", cloudData ) );
    ASSERT_TRUE( json_get_obj( result, "meshbotTemplates", meshbotTemplates ) );

    EXPECT_EQ( json_get_str( result, "status" ), "unknown" );
    EXPECT_EQ( json_get_int( result, "deployed" ), 0 );

    ASSERT_TRUE( json_contains( cloudData, "status" ) );
    ASSERT_TRUE( json_contains( cloudData, "currentVersion" ) );
    ASSERT_TRUE( json_contains( cloudData, "fileVersion" ) );
    ASSERT_TRUE( json_contains( cloudData, "url" ) );
    ASSERT_TRUE( json_contains( cloudData, "s3Id" ) );

    ASSERT_TRUE( json_contains( meshbotTemplates, "status" ) );
    ASSERT_TRUE( json_contains( meshbotTemplates, "currentVersion" ) );
    ASSERT_TRUE( json_contains( meshbotTemplates, "fileVersion" ) );
    ASSERT_TRUE( json_contains( meshbotTemplates, "url" ) );
    ASSERT_TRUE( json_contains( meshbotTemplates, "s3Id" ) );
}

TEST_F( HubLocalCloudDataUpdateTest, hubOfflineCloudDataInfoGetError ) {
    nma().getDBClient().update( "db.firmware.local.notifications.data",
                                JSON( "_id"
                                      << "tcds_data_info" ),
                                JSON( "$set" << JSON( "process_error"
                                                      << "testing tcds process error" ) ) );
    JsonObject error, result, cloudData, meshbotTemplates;

    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.offline.cloud.data.info.get" }, { "params", {} } } );

    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );

    EXPECT_EQ( json_get_str( error, "data" ), "local.notification.tcds.data.processing.error" );
    EXPECT_EQ( json_get_str( error, "message" ), "Local notification tcds data got error during deploying process" );
    EXPECT_EQ( json_get_str( error, "reason" ), "testing tcds process error" );
}

}  // namespace Tests
}  // namespace HubPlatform
