#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "cloud_storage/cloud_storage_impl.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. lua cloud storage
DONE        1.0 fetch token Failed
DONE        1.1 fetch token Success
DONE        1.2 fetch endpoints Success
DONE        1.3 fetch endpoints Failed
DONE        1.4 fetch endpoint Success
DONE        1.5 fetch endpoint with options Success
DONE        1.6 fetch endpoint Failed
DONE        1.7 fetch unknown endpoint

*/

namespace HubPlatform {
namespace Tests {

namespace Resources {
const Json storageControllerToken = R"({
    "endpoints": {
        "download": [
            "https://download-cloud-qa.ezlo.com"
        ],
        "upload": [
            "https://upload-cloud-qa.ezlo.com"
        ],
        "cloud_api": [
            "https://cloud-api-cloud-qa.ezlo.com",
            "https://cloud-api-1-cloud-qa.ezlo.com",
            "https://cloud-api-2-cloud-qa.ezlo.com"
        ],
        "unknown": [
            "https://unknown-cloud-qa.ezlo.com"
        ]
    },
    "token": {
        "expires_ts": 2147483647,
        "string": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
    },
    "xapp": "xapp-value-uuid"
} )"_json;

constexpr char TokenData[] = "token_data";
constexpr char EndPointsData[] = "endpoints_data";
constexpr char Url[] = "url";
constexpr char Token[] = "token";
constexpr char TokenExpirationDate[] = "expires_at";
constexpr char XApp[] = "x_app";
}  // namespace Resources

class CloudStorageLuadTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    void insertControllerTokenRecord();

    JsonObject checkUiBroadcast( int line ) {
        JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( line ) );
        EXPECT_EQ( broadcast.empty(), false );
        JsonObject params;
        EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << getCodeLineString( line );
        return params;
    }
};

void CloudStorageLuadTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void CloudStorageLuadTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void CloudStorageLuadTests::insertControllerTokenRecord() {
    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    controllerToken.fromApiRecord( Resources::storageControllerToken );

    ControllerToken::insertDbRecord( nma().getDBClient(), controllerToken );
}

static char fetchTokenScript[] = "HUB:cloud_storage/scripts/fetch_token.lua";

TEST_F( CloudStorageLuadTests, fetchTokenFailed ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_token";
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( CloudStorageLuadTests, fetchTokenSuccess ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_token";
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    JsonObject tokenData;
    json_get_obj( broadcast, Resources::TokenData, tokenData );
    ASSERT_FALSE( tokenData.empty() );
    EXPECT_EQ( json_get_str( tokenData, Resources::Token ), "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" );
    EXPECT_EQ( json_get_int( tokenData, Resources::TokenExpirationDate ), 2147483647 );
    EXPECT_EQ( json_get_str( tokenData, Resources::XApp ), "xapp-value-uuid" );
}

TEST_F( CloudStorageLuadTests, fetchEndPointsSuccess ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_endpoints";
    request[ "params" ][ "scriptParams" ][ "table" ] = JSON( "upload" << true << "download" << true );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    JsonObject endPointData;
    json_get_obj( broadcast, Resources::EndPointsData, endPointData );
    ASSERT_TRUE( json_contains( endPointData, "upload" ) );
    ASSERT_TRUE( json_contains( endPointData, "download" ) );

    request[ "params" ][ "scriptParams" ][ "table" ] = JSON( "upload" << true << "download" << false );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    json_get_obj( broadcast, Resources::EndPointsData, endPointData );
    ASSERT_TRUE( json_contains( endPointData, "upload" ) );
    ASSERT_FALSE( json_contains( endPointData, "download" ) );

    request[ "params" ][ "scriptParams" ][ "table" ] =
        JSON( "upload" << true << "download" << false << "cloud_api" << true );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    json_get_obj( broadcast, Resources::EndPointsData, endPointData );
    ASSERT_TRUE( json_contains( endPointData, "upload" ) );
    ASSERT_FALSE( json_contains( endPointData, "download" ) );
    ASSERT_TRUE( json_contains( endPointData, "cloud_api" ) );

    request[ "params" ][ "scriptParams" ][ "table" ] =
        JSON( "upload" << true << "download" << false << "cloud_api" << true << "unknown" << true );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    json_get_obj( broadcast, Resources::EndPointsData, endPointData );
    ASSERT_TRUE( json_contains( endPointData, "upload" ) );
    ASSERT_FALSE( json_contains( endPointData, "download" ) );
    ASSERT_TRUE( json_contains( endPointData, "cloud_api" ) );
    ASSERT_TRUE( json_contains( endPointData, "unknown" ) );
}

TEST_F( CloudStorageLuadTests, fetchEndPointsFailed ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_endpoints";
    request[ "params" ][ "scriptParams" ][ "table" ] = JSON_EMPTY_OBJ;
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( CloudStorageLuadTests, fetchEndPointSuccess ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_endpoint";
    request[ "params" ][ "scriptParams" ][ "name" ] = "cloud_api";
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    auto url = json_get_str( broadcast, Resources::Url );
    EXPECT_EQ( url, "https://cloud-api-cloud-qa.ezlo.com" );
}

TEST_F( CloudStorageLuadTests, fetchEndPointSuccessWithOptions ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_endpoint";
    request[ "params" ][ "scriptParams" ][ "name" ] = "cloud_api";
    request[ "params" ][ "scriptParams" ][ "options" ] = JSON( "index" << 2 );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    auto url = json_get_str( broadcast, Resources::Url );
    EXPECT_EQ( url, "https://cloud-api-1-cloud-qa.ezlo.com" );
}

TEST_F( CloudStorageLuadTests, fetchEndPointFailed ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_endpoint";
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( CloudStorageLuadTests, subscribeEvent ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "subscribe";
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    {
        nma().sendBroadcast( HUB_STORAGE_CONTROLLER_TOKEN_UPDATED, JSON_EMPTY_OBJ );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.storage_controller_token.updated" );

        broadcast = checkUiBroadcast( __LINE__ );
        EXPECT_EQ( json_get_str( broadcast, "event" ), "token_updated" );
        EXPECT_EQ( json_get_str( broadcast, "plugin" ), "cloud_storage" );
    }

    {
        nma().sendBroadcast( HUB_STORAGE_CONTROLLER_TOKEN_EXPIRED, JSON_EMPTY_OBJ );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.storage_controller_token.expired" );

        broadcast = checkUiBroadcast( __LINE__ );
        EXPECT_EQ( json_get_str( broadcast, "event" ), "token_expired" );
        EXPECT_EQ( json_get_str( broadcast, "plugin" ), "cloud_storage" );
    }
}

TEST_F( CloudStorageLuadTests, fetchUnknownEndPoint ) {
    insertControllerTokenRecord();
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fetchTokenScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;
    request[ "params" ][ "scriptParams" ][ "command" ] = "fetch_endpoint";
    request[ "params" ][ "scriptParams" ][ "name" ] = "unknown";
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    auto url = json_get_str( broadcast, Resources::Url );
    EXPECT_EQ( url, "https://unknown-cloud-qa.ezlo.com" );
}

}  // namespace Tests
}  // namespace HubPlatform
