#include "bus/base_bus.h"

#include "common_utils/cpp14.h"
#include <gtest/gtest.h>
#include "uid/handlers/hub_video_forward_handler.h"

#include <vector>

namespace HubPlatform {
namespace Uid {
namespace UT {

/*

TESTPLAN

DONE    1. Hub.videoforward api dispatching tests
DONE        1.1 Select API version by setting, empty payload
DONE        1.2 Select API version by setting=auto, payload for api=1.0
DONE        1.3 Select API version by setting=auto, payload for api=2.0

*/

class VideoForwardTests : public testing::Test {};

TEST_F( VideoForwardTests, selectApiVersionBySetting ) /* 1.1 */ {
    std::string version;

    Json commandParams = JSON_EMPTY_OBJ;

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "1", "", commandParams );
    EXPECT_EQ( version, "1.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "1", "1.0", commandParams );
    EXPECT_EQ( version, "1.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "1", "2.0", commandParams );
    EXPECT_EQ( version, "2.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "2", "", commandParams );
    EXPECT_EQ( version, "2.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "2", "1.0", commandParams );
    EXPECT_EQ( version, "2.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "2", "2.0", commandParams );
    EXPECT_EQ( version, "2.0" );
}

TEST_F( VideoForwardTests, autoSelectByPayloadApi1 ) /* 1.2 */ {
    std::string version;

    Json commandVideoforward1 = R"(
    {
        "id": "_id_",
        "method": "hub.videoforward",
        "params": {
            "data": {
                "action": "initiate_to_start_stream",
                "controller_uuid": "2e5df0b0-5fe5-11eb-9b3f-7bbeb0523980",
                "identity": "657a3d8e5ab7be145bb6501e",
                "req_rtsp_link": true,
                "rnd_client_id": 987,
                "rtsp_valid_for": 500,
                "with_remote_offer": true
            },
            "target": {
                "identity": "657a3d8e5ab7be145bb6501e",
                "parent_uuid": "2e5df0b0-5fe5-11eb-9b3f-7bbeb0523980"
            }
        }
    })"_json;
    Json commandParams = commandVideoforward1[ "params" ];

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "auto", "", commandParams );
    EXPECT_EQ( version, "1.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "auto", "1.0", commandParams );
    EXPECT_EQ( version, "1.0" );
}

TEST_F( VideoForwardTests, autoSelectByPayloadApi2 ) /* 1.3 */ {
    std::string version;

    auto commandVideoforward2 = R"(
    {
        "id":"_id_",
        "method":"hub.videoforward",
        "params":{
            "action":"camera.requestanswer",
            "params":{
                "sdp":"===",
                "suuid":"suuid",
                "type":"offer"
            }
        }
    })"_json;
    Json commandParams = commandVideoforward2[ "params" ];

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "auto", "", commandParams );
    EXPECT_EQ( version, "2.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "auto", "1.0", commandParams );
    EXPECT_EQ( version, "2.0" );

    version = HubVideoForwardHandler::selectApiVersionBySettingOrPayload( "auto", "2.0", commandParams );
    EXPECT_EQ( version, "2.0" );
}

TEST_F( VideoForwardTests, selectTargetGroup ) {
    auto mqttGroup = HubVideoForwardHandler::selectTargetGroup( "1.0" );
    ASSERT_EQ( mqttGroup, "webrtc" );

    mqttGroup = HubVideoForwardHandler::selectTargetGroup( "2.0" );
    ASSERT_EQ( mqttGroup, "mqtt.webrtc" );
}

}  // namespace UT
}  // namespace Uid
}  // namespace HubPlatform