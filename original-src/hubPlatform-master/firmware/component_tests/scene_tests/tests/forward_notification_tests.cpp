#include "fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "database_client/dbqueryparser.h"
#include <gtest/gtest.h>

/*
TESTPLAN
DONE    1. Request success
DONE    2. Request fail
*/

namespace HubPlatform {
namespace Tests {
const std::string LocalNotificationsDB = HaLocalNotificationsDataDbName;
class ForwardNotificationTests : public ScenesExpressionsFixture {
public:
    ForwardNotificationTests();
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();
    std::string createCompareNumbers( const std::string& itemId );
    std::string createForwardNotificationThenBlock( const std::string& itemId, const std::string& deviceId );
    void checkRequestMessage( const JsonObject& data, int codeLine );
    void fillTcdsDBRecords();
    void clearTcdsDBRecords();
    void checkCloudRequestWithResponse( const JsonObject& response, bool sussess );

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
};

ForwardNotificationTests::ForwardNotificationTests()
: m_remote( makeRemote( nma() ) ) {}

void ForwardNotificationTests::SetUp() {
    ScenesExpressionsFixture::SetUp();
    fillTcdsDBRecords();
}

void ForwardNotificationTests::TearDown() {
    ScenesExpressionsFixture::TearDown();
    clearTcdsDBRecords();
}

void ForwardNotificationTests::SetUpTestCase() {}

void ForwardNotificationTests::fillTcdsDBRecords() {
    nma().getDBClient().insert( LocalNotificationsDB,
                                JSON( "_id"
                                      << "active_cloud_data_info"
                                      << "s3_id"
                                      << "s3_id_cloud_value" ) );

    nma().getDBClient().insert( LocalNotificationsDB,
                                JSON( "_id"
                                      << "active_meshbots_data_info"
                                      << "s3_id"
                                      << "s3_id_meshbot_value" ) );
    nma().getDBClient().insert( LocalNotificationsDB,
                                JSON( "_id"
                                      << "files"
                                      << JSON( "key1"
                                               << "val1" ) ) );
}

void ForwardNotificationTests::clearTcdsDBRecords() {
    nma().getDBClient().clear( LocalNotificationsDB );
}

void ForwardNotificationTests::checkRequestMessage( const JsonObject& data, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );

    EXPECT_EQ( json_get_int( data, "api" ), 1 );
    EXPECT_EQ( json_get_str( data, "method" ), "cloud.controller_notification_send" );
    EXPECT_FALSE( json_get_str( data, "id" ).empty() );

    JsonObject params;
    EXPECT_TRUE( json_get_obj( data, "params", params ) );

    EXPECT_FALSE( json_get_str( params, "trace_id" ).empty() );
    EXPECT_EQ( json_get_str( params, "template_uuid" ), "templateId" );

    JsonObject device;
    EXPECT_TRUE( json_get_obj( params, "device", device ) );
    EXPECT_FALSE( device.empty() );
    JsonArray items;
    EXPECT_TRUE( json_get_arr( params, "items", items ) );
    EXPECT_EQ( items.size(), 1 );

    JsonObject item = items.front();
    EXPECT_TRUE( item.is_object() );
    EXPECT_FALSE( item.empty() );

    JsonObject scene;
    EXPECT_TRUE( json_get_obj( params, "scene", scene ) );
    EXPECT_FALSE( scene.empty() );

    JsonArray targets;
    EXPECT_TRUE( json_get_arr( params, "targets", targets ) );
    EXPECT_EQ( targets.size(), 3 );

    JsonObject files;
    EXPECT_TRUE( json_get_obj( params, "files", files ) );
    EXPECT_FALSE( files.empty() );
    const auto filesExpected = R"({"meshbots_ds3_id":"s3_id_meshbot_value","s3_id":"s3_id_cloud_value"})";
    EXPECT_EQ( files.dump(), filesExpected );

    JsonObject meta;
    EXPECT_TRUE( json_get_obj( params, "meta", meta ) );
    EXPECT_FALSE( meta.empty() );

    JsonObject broadcast;
    EXPECT_TRUE( json_get_obj( meta, "broadcast", broadcast ) );
    EXPECT_FALSE( broadcast.empty() );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" );
    EXPECT_FALSE( json_get_str( broadcast, "id" ).empty() );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_FALSE( result.empty() );
}

std::string ForwardNotificationTests::createCompareNumbers( const std::string& itemId ) {
    return replaceTokensInString( compareNumbersWhenBlock,
                                  { { "__COMPARATOR__", ">" },
                                    { "__VALUE__", "50" },
                                    { "__TYPE__", "int" },
                                    { "__SOURCE_NAME__", "item" },
                                    { "__SOURCE_TYPE__", "item" },
                                    { "__SOURCE_VALUE__", "\"" + itemId + "\"" } } );
}

std::string ForwardNotificationTests::createForwardNotificationThenBlock( const std::string& itemId,
                                                                          const std::string& deviceId ) {
    return replaceTokensInString( forwardNotificationThenBlock,
                                  { { "__TEMPLATE__", "templateId" },
                                    { "__TARGETS__", R"(["1","2","3"])" },
                                    { "__DEVICE_ID__", deviceId },
                                    { "__ITEM_ID__", itemId } } );
}

void ForwardNotificationTests::checkCloudRequestWithResponse( const JsonObject& response, bool success ) {
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto actionBlock = createForwardNotificationThenBlock( device.m_itemId, device.m_deviceId );
    std::string whenBlock = createCompareNumbers( device.m_itemId );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "ForwardNotificationTest" },
                                    { "__WHEN_BLOCKS__", std::move( whenBlock ) },
                                    { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    SceneID sceneId = createScene( request );

    auto goodCloudResponse = [this, response]( rpc::Message& rpcMessage ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        checkRequestMessage( joMessage, __LINE__ );
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = response;
        joAnswer[ "error" ] = nullptr;
    };

    nma().setCloudMethod( "cloud.controller_notification_send", goodCloudResponse );

    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    setItemValue( coredRemote, device.m_itemId, 70, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, success ? "finished" : "failed", __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ForwardNotificationTests, sendRequestSuccess ) {  // 1
    checkCloudRequestWithResponse( JSON( "status" << 1 << "complete" << 1 ), true );
}

TEST_F( ForwardNotificationTests, sendRequestFail ) {  // 2
    checkCloudRequestWithResponse( JSON( "status" << 1 << "complete" << 0 ), false );
    checkCloudRequestWithResponse( JSON( "status" << 0 << "complete" << 0 ), false );
    checkCloudRequestWithResponse( JSON( "status" << 0 << "complete" << 1 ), false );
}

}  // namespace Tests
}  // namespace HubPlatform
