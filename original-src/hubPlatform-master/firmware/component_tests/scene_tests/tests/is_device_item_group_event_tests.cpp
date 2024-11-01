#include "common_test_resources/fixtures/device_item_groups_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN


DONE    1  isDeviceItemGroup
DONE        1.1 isDeviceItemGroup For Integer Item Update
DONE        1.2 isDeviceItemGroup For Boolean Item Update
DONE        1.3 isDeviceItemGroup For String Item Update
DONE        1.4 isDeviceItemGroup For Dictionary Item Update
DONE        1.5 isDeviceItemGroup not trigger
DONE        1.6 isDeviceItemGroup with AND operator
DONE        1.7 isDeviceItemGroup checking state test

*/

class IsDeviceItemGroupEventTests : public DeviceItemGroupsFixture {
public:
    void SetUp() override;
    void TearDown() override;
};

void IsDeviceItemGroupEventTests::SetUp() {
    DeviceItemGroupsFixture::SetUp();
    callingFile( __FILE__ );
}

void IsDeviceItemGroupEventTests::TearDown() {
    clearExistingDevices( __LINE__ );
    DeviceItemGroupsFixture::TearDown();
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupForIntegerItemUpdate ) {  // 1.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeIntItem( deviceId, testItemName, 0, __LINE__ );

    auto block = replaceTokensInString( isDeviceItemGroupEvent,
                                        { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceItemGroupEvent" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( m_remote, itemId, 20, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( m_remote, itemId, 30, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupForBooleanItemUpdate ) {  // 1.2
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeBooleanItem( deviceId, testItemName, false, __LINE__ );

    auto block = replaceTokensInString( isDeviceItemGroupEvent,
                                        { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceItemGroupEvent" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( m_remote, itemId, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( m_remote, itemId, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupForStringItemUpdate ) {  // 1.3
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "string" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeStringItem( deviceId, testItemName, "", __LINE__ );

    auto block = replaceTokensInString( isDeviceItemGroupEvent,
                                        { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceItemGroupEvent" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( m_remote, itemId, "hello", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( m_remote, itemId, "world", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupForDictionaryItemUpdate ) {  // 1.4
    const std::string deviceCategory = "door_lock";
    const std::string deviceSubcategory = "";
    const std::string testItemName = "test_device";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForDictionaryItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto dictionaryType = CoreD::Model::ValueType::makeDictionaryType( CoreD::Model::ValueType::Enum::Token );
    auto createItemGroupRequest = replaceTokens( scenesItemGroupCreate,
                                                 { { "__NAME__", "testItemGroup" },
                                                   { "__ITEM_NAME__", testItemName },
                                                   { "__VALUE_TYPE__", dictionaryType.toString() } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = addDevice( m_remote,
                               "door",           /*name*/
                               "doorlock",       /*type*/
                               "297_32770_1536", /*deviceTypeId*/
                               "door_lock",      /*category*/
                               "",               /*subcategory*/
                               false,
                               __LINE__ );

    CoreDRemoteInterface::CoreDInterface::ItemID itemId;
    itemId = addItem( m_remote,
                      deviceId,
                      "test_device",
                      CoreD::Model::ValueType::makeDictionaryType( CoreD::Model::ValueType::Enum::Token ),
                      true,
                      true,
                      __LINE__ );

    auto addDictionaryValue = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "token" }, { "__VALUE__", "\"dictionary\"" } } )[ "params" ];

    KeyJsonList fields = {
        { "operation", "added" },
        { "element",
          JSON( "value"
                << "" ) },
        { "_id", itemId },
        { "deviceId", deviceId },
        { "deviceName", "door" },
        { "deviceCategory", "door_lock" },
        { "deviceSubcategory", "" },
        { "roomName", "" },
        { "name", "test_device" },
    };

    auto addItemDictionaryValue =
        [&fields, &addDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            addDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( m_remote.addItemDictionaryValue( addDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "added";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto block = replaceTokensInString( isDeviceItemGroupEvent,
                                        { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceItemGroupEvent" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    addItemDictionaryValue( itemId, "1", "dictionary", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    addItemDictionaryValue( itemId, "2", "dictionary", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupNotTrigger ) {  // 1.5
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    {
        auto createItemGroupRequest = replaceTokens(
            scenesItemGroupCreate,
            { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
        json_remove( createItemGroupRequest[ "params" ], "enum" );

        auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

        auto deviceId = makeDevice( "test_device", "device_subcategory", __LINE__ );
        auto itemId = makeIntItem( deviceId, "test_item", 0, __LINE__ );

        auto block = replaceTokensInString(
            isDeviceItemGroupEvent, { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

        auto request = replaceTokens(
            createSceneRequest,
            { { "__NAME__", "isDeviceItemGroupEvent" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

        SceneID sceneId = createScene( request, __LINE__ );
        LOGGER_TRACE( "Scene created, id: " + sceneId );

        setItemValue( m_remote, itemId, 20, __LINE__ );
        checkNoBroadcastReceived( "No scene trigger broadcast", __LINE__ );

        deleteScene( sceneId, __LINE__ );
        deleteItemGroup( itemGroupId, __LINE__ );
        deleteDeviceGroup( deviceGroupId, __LINE__ );
    }
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupAndOperator ) {  // 1.6
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate_2";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "test_device_group" << JSON_ARRAY( "test_subcategory" ) ) );
    auto deviceGroupId2 = createDeviceGroup( createGroupRequest, __LINE__ );

    {
        auto createItemGroupRequest = replaceTokens(
            scenesItemGroupCreate,
            { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
        json_remove( createItemGroupRequest[ "params" ], "enum" );

        auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

        createItemGroupRequest = replaceTokens(
            scenesItemGroupCreate,
            { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", "test_item" }, { "__VALUE_TYPE__", "int" } } );
        json_remove( createItemGroupRequest[ "params" ], "enum" );

        auto itemGroupId2 = createItemGroup( createItemGroupRequest, __LINE__ );

        auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
        auto itemId = makeIntItem( deviceId, testItemName, 0, __LINE__ );

        auto deviceId2 = makeDevice( "test_device_group", "test_subcategory", __LINE__ );
        auto itemId2 = makeIntItem( deviceId, "test_item", 0, __LINE__ );

        auto block1 = replaceTokensInString(
            isDeviceItemGroupEvent, { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

        auto block2 = replaceTokensInString(
            isDeviceItemGroupEvent, { { "__DEVICE_GROUP__", deviceGroupId2 }, { "__ITEM_GROUP__", itemGroupId2 } } );

        auto notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", block2 } } );

        block1 += ",";
        block1.append( std::move( notOperator ) );
        auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", block1 } } );

        auto request = replaceTokens( createSceneRequest,
                                      { { "__NAME__", "isDeviceItemGroupEvent" },
                                        { "__WHEN_BLOCKS__", andOperator },
                                        { "__THEN_BLOCKS__", "" } } );

        SceneID sceneId = createScene( request, __LINE__ );
        LOGGER_TRACE( "Scene created, id: " + sceneId );

        setItemValue( m_remote, itemId, 20, __LINE__ );
        testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
        testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

        setItemValue( m_remote, itemId2, 10, __LINE__ );
        setItemValue( m_remote, itemId, 10, __LINE__ );
        checkNoBroadcastReceived( "No scene trigger broadcast", __LINE__ );

        deleteScene( sceneId, __LINE__ );
        deleteItemGroup( itemGroupId, __LINE__ );
        deleteDeviceGroup( deviceGroupId, __LINE__ );
        deleteItemGroup( itemGroupId2, __LINE__ );
        deleteDeviceGroup( deviceGroupId2, __LINE__ );
    }
}

TEST_F( IsDeviceItemGroupEventTests, isDeviceItemGroupStateCheck ) {  // 1.7
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "isDeviceItemGroupForItemUpdate";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    {
        auto createItemGroupRequest = replaceTokens(
            scenesItemGroupCreate,
            { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
        json_remove( createItemGroupRequest[ "params" ], "enum" );

        auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

        auto deviceId = makeDevice( "test_device", "device_subcategory", __LINE__ );
        auto itemId = makeIntItem( deviceId, testItemName, 0, __LINE__ );
        auto itemId2 = makeIntItem( deviceId, "test_item", 0, __LINE__ );

        auto block = replaceTokensInString(
            isDeviceItemGroupEvent, { { "__DEVICE_GROUP__", deviceGroupId }, { "__ITEM_GROUP__", itemGroupId } } );

        auto request = replaceTokens(
            createSceneRequest,
            { { "__NAME__", "isDeviceItemGroupEvent" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

        SceneID sceneId = createScene( request, __LINE__ );
        LOGGER_TRACE( "Scene created, id: " + sceneId );

        setItemValue( m_remote, itemId, 20, __LINE__ );
        testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
        testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
        Json sceneStatus = getSceneStatus( sceneId, __LINE__ );

        {
            std::string state;
            std::vector< Json > statePath{ "result", "status", "when", 0, "state" };
            ASSERT_TRUE( json_get( sceneStatus, statePath, state ) );
            EXPECT_EQ( state, "true" );
        }

        setItemValue( m_remote, itemId2, 10, __LINE__ );
        checkNoBroadcastReceived( "No scene trigger broadcast", __LINE__ );
        sceneStatus = getSceneStatus( sceneId, __LINE__ );

        {
            std::string state;
            std::vector< Json > statePath{ "result", "status", "when", 0, "state" };
            ASSERT_TRUE( json_get( sceneStatus, statePath, state ) );
            EXPECT_EQ( state, "false" );
        }

        deleteScene( sceneId, __LINE__ );
        deleteItemGroup( itemGroupId, __LINE__ );
        deleteDeviceGroup( deviceGroupId, __LINE__ );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
