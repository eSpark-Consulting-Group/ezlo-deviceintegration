#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include "cored/model/values/value_factory.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1 Create item group
DONE        1.1 Create item group
DONE    2 Delete item group
DONE        2.1 Delete item group
DONE        2.2 Delete persistent item group error
DONE        2.3 Delete persistent item group with 'force' flag
DONE        2.4 Delete item group error
DONE    3 Update item group
DONE        3.1 Update item group
DONE    4 Item Group List
DONE        4.1 Item group list
DONE        4.2 Item group list version
DONE    5 Item Group Get
DONE        5.1 Item group get
DONE        5.2 Item group get error


*/

namespace Broadcasts {
const char* const ItemGroupCreated = "hub.item.group.created";
const char* const ItemGroupDeleted = "hub.item.group.deleted";
const char* const ItemGroupUpdated = "hub.item.group.updated";
}  // namespace Broadcasts

class SceneItemGroupTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;

    static Json makeItemGroupCreateRequest( std::string name,
                                            std::string itemName = "dimmer",
                                            std::string valueType = "token" );
    static Json makeItemGroupDeleteRequest( std::string id, bool force = false );

    void deleteGroup( const std::string& id, bool force, int codeLine );
};

void SceneItemGroupTests::SetUpTestCase() {}

void SceneItemGroupTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void SceneItemGroupTests::deleteGroup( const std::string& id, bool force, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto request = makeItemGroupDeleteRequest( id, force );
    sendRequestAndCheckAnswer( request, codeLine );
    checkBroadcast( id, Broadcasts::ItemGroupDeleted, {}, codeLine );
}

Json SceneItemGroupTests::makeItemGroupCreateRequest( std::string name, std::string itemName, std::string valueType ) {
    return replaceTokens( scenesItemGroupCreate,
                          { { "__NAME__", name }, { "__ITEM_NAME__", itemName }, { "__VALUE_TYPE__", valueType } } );
}

Json SceneItemGroupTests::makeItemGroupDeleteRequest( std::string id, bool force ) {
    return replaceTokens( scenesItemGroupDelete,
                          { { "__GROUP_ID__", id }, { "__FORCE__", force ? "true" : "false" } } );
}

TEST_F( SceneItemGroupTests, createItemGroup ) {  // 1.1
    auto request = makeItemGroupCreateRequest( "simple" );
    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.item.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    ASSERT_FALSE( id.empty() ) << "_id shouldn't be empty";
    deleteGroup( id, false, __LINE__ );
}

TEST_F( SceneItemGroupTests, deleteItemGroup ) {  // 2.1
    auto request = makeItemGroupCreateRequest( "toDelete" );
    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.item.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    ASSERT_FALSE( id.empty() ) << "_id shouldn't be empty";
    compareBroadcastFields( broadcast,
                            "",
                            Broadcasts::ItemGroupCreated,
                            { { Resources::CheckBroadcast::Has, "_id" } },
                            NMA_TEST_MAKE_MESSAGE( "" ) );

    deleteGroup( id, false, __LINE__ );
}

TEST_F( SceneItemGroupTests, deletePersistentItemGroup ) {  // 2.2
    auto request = replaceTokensInString( scenesPersistentItemGroupCreate,
                                          { { "__NAME__", "nameDeleted1" }, { "__PERSISTENT__", "true" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.item.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    ASSERT_FALSE( id.empty() ) << "_id shouldn't be empty";

    request = replaceTokensInString( scenesItemGroupDelete, { { "__GROUP_ID__", id }, { "__FORCE__", "false" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.delete.group.impossible" );
    EXPECT_EQ( json_get_str( error, "message" ), "Can't delete persistent group" );

    deleteGroup( id, true, __LINE__ );
}

TEST_F( SceneItemGroupTests, deleteForcePersistentItemGroup ) {  // 2.3
    auto request = replaceTokensInString( scenesPersistentItemGroupCreate,
                                          { { "__NAME__", "nameDeleted2" }, { "__PERSISTENT__", "true" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.item.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    request = replaceTokensInString( scenesItemGroupDelete, { { "__GROUP_ID__", id }, { "__FORCE__", "true" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    broadcast = waitForBroadcast( "hub.item.group.deleted", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupDeleted );
}

TEST_F( SceneItemGroupTests, deleteItemGroupError ) {  // 2.4
    auto request =
        replaceTokensInString( scenesItemGroupDelete, { { "__GROUP_ID__", "badId" }, { "__FORCE__", "false" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.not.exist" );
    EXPECT_EQ( json_get_str( error, "field" ), "_id" );
}

TEST_F( SceneItemGroupTests, updateItemGroup ) {  // 3.1
    Json request = makeItemGroupCreateRequest( "nameToUpdate" );
    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.item.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    request = Json::parse(
        replaceTokensInString( scenesItemGroupUpdate, { { "__GROUP_ID__", id }, { "__NAME__", "nameToUpdate" } } ) );
    sendRequestAndCheckAnswer( request, __LINE__ );
    broadcast = waitForBroadcast( "hub.item.group.updated", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupUpdated );
    request = Json::parse(
        replaceTokensInString( scenesItemGroupUpdate, { { "__GROUP_ID__", id }, { "__NAME__", "nameToUpdate" } } ) );
    sendRequestAndCheckAnswer( request, __LINE__ );
    request = Json::parse(
        replaceTokensInString( scenesItemGroupUpdate, { { "__GROUP_ID__", id }, { "__NAME__", "nameUpdated" } } ) );
    sendRequestAndCheckAnswer( request, __LINE__ );
    broadcast = waitForBroadcast( "hub.item.group.updated", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupUpdated );

    deleteGroup( id, false, __LINE__ );
}

TEST_F( SceneItemGroupTests, itemGroupsListTest ) {  // 4.1
    Json request = makeItemGroupCreateRequest( "listName" );

    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( Broadcasts::ItemGroupCreated, getCodeLineString( __LINE__ ) );
    auto itemGroupId = json_get_str( broadcast[ "result" ], "_id" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupCreated );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( itemGroupsList ) );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& itemGroups = answer[ "result" ][ "itemGroups" ];
    EXPECT_FALSE( itemGroups.empty() );
    bool idExist = false;
    for( auto& group : itemGroups ) {
        EXPECT_FALSE( group.empty() );
        const auto id = json_get_str( group, "_id" );
        if( id == itemGroupId ) {
            idExist = true;
            EXPECT_EQ( json_get_str( group, "name" ), "listName" );
            EXPECT_EQ( json_get_str( group, "valueType" ), "token" );
            EXPECT_EQ( json_get_bool( group, "persistent" ), false );
            EXPECT_EQ( json_get_bool( group, "hasGetter" ), true );
            EXPECT_EQ( json_get_bool( group, "hasSetter" ), false );
        }
    }

    EXPECT_TRUE( idExist );

    deleteGroup( itemGroupId, false, __LINE__ );
}

TEST_F( SceneItemGroupTests, itemGroupsListVersionTest ) {  // 4.2

    std::string version;
    std::string itemGroupId;
    {
        Json request = makeItemGroupCreateRequest( "listName" );

        sendRequestAndCheckAnswer( request, __LINE__ );
        JsonObject broadcast = waitForBroadcast( Broadcasts::ItemGroupCreated, getCodeLineString( __LINE__ ) );
        itemGroupId = json_get_str( broadcast[ "result" ], "_id" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupCreated );
        auto answer = nma().sendtoHubAndGetAnswer( Json::parse( itemGroupsList ) );
        const auto& error = answer[ "error" ];
        EXPECT_TRUE( error.empty() );
        const auto& itemGroups = answer[ "result" ][ "itemGroups" ];
        version = json_get_str( answer[ "result" ], "version" );
        EXPECT_FALSE( itemGroups.empty() );
        bool idExist = false;
        for( auto& group : itemGroups ) {
            EXPECT_FALSE( group.empty() );
            const auto id = json_get_str( group, "_id" );
            if( id == itemGroupId ) {
                idExist = true;
                EXPECT_EQ( json_get_str( group, "name" ), "listName" );
                EXPECT_EQ( json_get_str( group, "valueType" ), "token" );
                EXPECT_EQ( json_get_bool( group, "persistent" ), false );
                EXPECT_EQ( json_get_bool( group, "hasGetter" ), true );
                EXPECT_EQ( json_get_bool( group, "hasSetter" ), false );
            }
        }

        EXPECT_TRUE( idExist );
    }

    auto request = replaceTokensInString( itemGroupsList, { { "__VERSION__", version } } );
    {
        auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
        const auto& error = answer[ "error" ];
        EXPECT_TRUE( error.empty() );
        const auto& itemGroups = answer[ "result" ][ "itemGroups" ];
        EXPECT_TRUE( itemGroups.empty() );
        EXPECT_EQ( answer[ "result" ][ "version" ], version );
    }

    deleteGroup( itemGroupId, false, __LINE__ );
}

TEST_F( SceneItemGroupTests, itemGroupGetTest ) {  // 5.1
    Json request = makeItemGroupCreateRequest( "getName" );

    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( Broadcasts::ItemGroupCreated, getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupCreated );
    auto itemGroupId = json_get_str( broadcast[ "result" ], "_id" );
    ASSERT_FALSE( itemGroupId.empty() ) << "_id shouldn't be empty";

    request = Json::parse( replaceTokensInString( itemGroupGet, { { "__GROUP_ID__", itemGroupId } } ) );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& result = answer[ "result" ];
    EXPECT_FALSE( result.empty() );
    EXPECT_EQ( json_get_str( result, "_id" ), itemGroupId );
    EXPECT_EQ( json_get_str( result, "name" ), "getName" );

    deleteGroup( itemGroupId, false, __LINE__ );
}

TEST_F( SceneItemGroupTests, itemGroupGetErrorTest ) {  // 5.2
    Json request = makeItemGroupCreateRequest( "getName1" );

    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( Broadcasts::ItemGroupCreated, getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), Broadcasts::ItemGroupCreated );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    ASSERT_FALSE( id.empty() ) << "_id shouldn't be empty";

    request = Json::parse( replaceTokensInString( itemGroupGet, { { "__GROUP_ID__", "badId" } } ) );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.not.exist" );
    EXPECT_EQ( json_get_str( error, "field" ), "_id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Record with the key does not exist" );

    deleteGroup( id, false, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
