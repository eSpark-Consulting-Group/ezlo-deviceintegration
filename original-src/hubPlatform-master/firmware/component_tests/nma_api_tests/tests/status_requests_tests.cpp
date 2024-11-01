#include "test_utils/component_tests/ws_server/test_server_peer.h"
#include "common_utils/firmware_info_helper.h"

#include "test_utils/component_tests/common_nma_test_fixture.h"

#include <gtest/gtest.h>

#define STRING( S ) STR( S )
#define STR( S ) #S

namespace HubPlatform {
namespace Tests {

class StatusRequestsTests : public CommonNmaTests {};

TEST_F( StatusRequestsTests, getInfoHubRequest ) {
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.info.get" }, { "params", JSON_EMPTY_OBJ } } );
    const auto resultField = answer[ "result" ];

    const auto firmwareVersion = json_get_str( resultField, "firmware" );
    EXPECT_EQ( firmwareVersion, FirmwareInfoHelper::getFirmwareVersionString() );

    const auto hubType = json_get_str( resultField, "model" );
    EXPECT_EQ( hubType, "local" );

    const auto hubSerial = json_get_str( resultField, "serial" );
    EXPECT_EQ( hubSerial, std::string{ __HUB_SERIAL_STR__ } );

    JsonObject localModeAccessKeys = JSON_EMPTY_OBJ;
    EXPECT_TRUE( json_get( resultField, { "localMode", "accessKeys" }, localModeAccessKeys ) );
    EXPECT_EQ( localModeAccessKeys.count( "hasCredentials" ), 1 );
    EXPECT_EQ( localModeAccessKeys.count( "version" ), 1 );
}

TEST_F( StatusRequestsTests, checkMethodNameInAnswer ) {
    const std::string methodName = "hub.info.get";
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", methodName }, { "params", JSON_EMPTY_OBJ } } );

    const std::string answerMethodName = json_get_str( answer, "method" );
    EXPECT_EQ( answerMethodName, methodName );
}

TEST_F( StatusRequestsTests, getHubStatusGetRequest ) {
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.status.get" }, { "params", JSON_EMPTY_OBJ } } );

    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    JsonObject controlGroups;
    if( json_get_obj( result, "controlGroups", controlGroups ) ) {
        JsonObject webrtc;
        if( json_get_obj( controlGroups, "webrtc", webrtc ) ) {
            JsonObject limits;
            EXPECT_TRUE( json_get_obj( webrtc, "limits", limits ) );
            JsonObject oom;
            EXPECT_TRUE( json_get_obj( webrtc, "oom", oom ) );
        }
    }
}

TEST_F( StatusRequestsTests, hubServiceInfoGet_UiService ) {
    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.service.info.get" }, { "params", { { "service", "ui_service" } } } } );
    const auto resultField = answer[ "result" ];

    LOGGER_DEBUG( "service.info.get/ui_service:" + cc::j( resultField ) );

    Json info;
    EXPECT_TRUE( json_get( resultField, "info", info ) );
    EXPECT_EQ( info.count( "httpServer" ), 1 );
    EXPECT_EQ( info.count( "nma" ), 1 );
    EXPECT_EQ( info.count( "localMode" ), 1 );
    EXPECT_EQ( info[ "localMode" ].count( "status" ), 1 );
}

TEST_F( StatusRequestsTests, AddonLuaMetrics ) {
    auto serviceChannel = "addon.lua";
    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.service.info.get" }, { "params", { { "service", serviceChannel } } } } );
    const auto resultField = answer[ "result" ];

    LOGGER_DEBUG( "service.info.get/" + cc::str( serviceChannel ) + ": " + cc::j( resultField ) );

    Json info;
    EXPECT_TRUE( json_get( resultField, "info", info ) );
    Json metrics, global, plugins;
    ASSERT_TRUE( json_get( info, "metrics", metrics ) );
    ASSERT_TRUE( json_get( metrics, "global", global ) );
    Log::info( "Global: " + cc::j( global ) );
    ASSERT_TRUE( json_get( metrics, "plugins", plugins ) );

    // Check metrics/global record schema
    EXPECT_TRUE( json_contains( global, "interpreterCreated" ) );
    EXPECT_TRUE( json_contains( global, "scriptAborted" ) );
    EXPECT_TRUE( json_contains( global, "scriptStarted" ) );
    EXPECT_TRUE( json_contains( global, "scriptFinished" ) );
    EXPECT_TRUE( json_contains( global, "scriptFailed" ) );
    EXPECT_TRUE( json_contains( global, "scriptTimeout" ) );
    EXPECT_TRUE( json_contains( global, "duration" ) );

    /** Test records for every required plugin. All required plugins should have startup.lua script configured for the
     * test to pass! */
    auto& app = nma();
    auto gateways = app.getRequiredGateways();
    for( auto& gatewayName : gateways ) {
        EXPECT_TRUE( json_contains( plugins, gatewayName ) ) << "Gateway not present: " << gatewayName;
    }

    EXPECT_EQ( metrics.count( "plugins" ), 1 );
}

}  // namespace Tests
}  // namespace HubPlatform
