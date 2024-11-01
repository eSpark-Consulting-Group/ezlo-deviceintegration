#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1. "value -> value" transition trigger scene execution
DONE    2. "value -> value" transition trigger "else" block
DONE    3. "any -> any" transition trigger "else" block

*/

class IsItemStateChangedTests : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }

    static Json createIsItemStateChangedWhenBlock( const std::string& itemId, int startValue, int finishValue ) {
        const KeyStringList replaceList = { { "__SOURCE_NAME__", "item" },
                                            { "__SOURCE_TYPE__", "item" },
                                            { "__SOURCE_VALUE__", quote( itemId ) },
                                            { "__START_TYPE__", "int" },
                                            { "__START_VALUE__", std::to_string( startValue ) },
                                            { "__FINISH_TYPE__", "int" },
                                            { "__FINISH_VALUE__", std::to_string( finishValue ) } };
        return replaceTokens( isItemStateChangedWhenBlock, replaceList );
    }

    static Json createIsItemStateChangedWhenBlock( const std::string& itemId ) {
        const KeyStringList replaceList = { { "__SOURCE_NAME__", "item" },           { "__SOURCE_TYPE__", "item" },
                                            { "__SOURCE_VALUE__", quote( itemId ) }, { "__START_TYPE__", "string" },
                                            { "__START_VALUE__", quote( "any" ) },   { "__FINISH_TYPE__", "string" },
                                            { "__FINISH_VALUE__", quote( "any" ) } };
        return replaceTokens( isItemStateChangedWhenBlock, replaceList );
    }
};

TEST_F( IsItemStateChangedTests, valueToValueTransitionTriggerSceneExecution ) {  // 1.
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    const std::string& itemId = device.m_itemId;
    const int startValue = 1;
    const int finishValue = 2;

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( createIsItemStateChangedWhenBlock( itemId, startValue, finishValue ) );

    const SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, itemId, Json( startValue ), __LINE__ );
    setItemValue( coreRemoteInterface, itemId, Json( finishValue ), __LINE__ );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );
}

TEST_F( IsItemStateChangedTests, valueToValueTransitionTriggerElseBlock ) {  // 2.
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto device2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    const int startValue = 1;
    const int finishValue = 2;
    const int elseBlockSetValue = 42;

    Json whenBlock = createIsItemStateChangedWhenBlock( device1.m_itemId, startValue, finishValue );
    Json elseBlock = replaceTokens( setItemValueThenBlock,
                                    { { "__ITEM_ID__", device2.m_itemId },
                                      { "__TYPE__", "int" },
                                      { "__VALUE__", std::to_string( elseBlockSetValue ) } } );

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( std::move( whenBlock ) );
    scene[ "params" ][ "else" ] = JSON_ARRAY_( std::move( elseBlock ) );

    const SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, device1.m_itemId, Json( 1 ), __LINE__ );
    setItemValue( coreRemoteInterface, device1.m_itemId, Json( 2 ), __LINE__ );

    checkBroadcasts( { [this, &sceneId, &scene]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, scene, sceneId, "started" );
                      },
                       [this, &sceneId, &scene]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, scene, sceneId, "finished" );
                       },
                       [this, &device2]( const Json& broadcast ) {
                           return isItemUpdateBroadcast( broadcast, device2.m_itemId, elseBlockSetValue );
                       } },
                     __LINE__ );
}

TEST_F( IsItemStateChangedTests, anyToAnyTransitionTriggerElseBlock ) {  // 3.
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto device2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    const int elseBlockSetValue = 42;

    Json whenBlock = createIsItemStateChangedWhenBlock( device1.m_itemId );
    Json elseBlock = replaceTokens( setItemValueThenBlock,
                                    { { "__ITEM_ID__", device2.m_itemId },
                                      { "__TYPE__", "int" },
                                      { "__VALUE__", std::to_string( elseBlockSetValue ) } } );

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( std::move( whenBlock ) );
    scene[ "params" ][ "else" ] = JSON_ARRAY_( std::move( elseBlock ) );

    const SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, device1.m_itemId, Json( 1 ), __LINE__ );

    checkBroadcasts( { [this, &sceneId, &scene]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, scene, sceneId, "started" );
                      },
                       [this, &sceneId, &scene]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, scene, sceneId, "finished" );
                       },
                       [this, &device2]( const Json& broadcast ) {
                           return isItemUpdateBroadcast( broadcast, device2.m_itemId, elseBlockSetValue );
                       } },
                     __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
