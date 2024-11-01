#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**

TESTPLAN

DONE    1. resetLatch
DONE        1.1 valid case
DONE        1.2 invalid params (reset non-existing block)
DONE    2. resetSceneLatches
DONE        2.1 valid case
DONE        2.2 invalid params (reset latches in non-existing scene)

 */

namespace HubPlatform {
namespace Tests {

class ResetLatchActionTest : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }

    static std::string createCompareNumbers( std::string itemId );
    static std::string createLatch( std::string nestedBlock );
    static Json createSceneJson( std::string whenBlocks );
};

std::string ResetLatchActionTest::createCompareNumbers( std::string itemId ) {
    return replaceTokensInString( compareNumbersWhenBlock,
                                  { { "__COMPARATOR__", ">" },
                                    { "__VALUE__", "50" },
                                    { "__TYPE__", "int" },
                                    { "__SOURCE_NAME__", "item" },
                                    { "__SOURCE_TYPE__", "item" },
                                    { "__SOURCE_VALUE__", "\"" + itemId + "\"" } } );
}

std::string ResetLatchActionTest::createLatch( std::string nestedBlock ) {
    return replaceTokensInString(
        functionOperatorWhenBlock,
        { { "__BLOCKS__", std::move( nestedBlock ) }, { "__FUNCTION__", R"("latch":{ "enabled":true })" } } );
}

Json ResetLatchActionTest::createSceneJson( std::string whenBlocks ) {
    return replaceTokens( createSceneRequest,
                          { { "__NAME__", "some_name" },
                            { "__EXEC_POLICY__", "ignore_result" },
                            { "__WHEN_BLOCKS__", std::move( whenBlocks ) },
                            { "__THEN_BLOCKS__", "" } } );
}

TEST_F( ResetLatchActionTest, resetLatch_validCase ) {  // 1.1
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    auto deviceWithItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    std::string latchBlock = createLatch( createCompareNumbers( deviceWithItem.m_itemId ) );
    Json scene1Json = createSceneJson( std::move( latchBlock ) );
    Json scene1Result;
    const SceneID scene1Id = createScene( scene1Json, scene1Result, __LINE__ );

    const Json& createdWhenBlocks = scene1Result[ "when" ];
    ASSERT_EQ( createdWhenBlocks.size(), 1 );
    const std::string latchId = createdWhenBlocks.front()[ "_id" ];

    setItemValue( coredRemote, deviceWithItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "started", __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "finished", __LINE__ );

    setItemValue( coredRemote, deviceWithItem.m_itemId, 10, __LINE__ );
    setItemValue( coredRemote, deviceWithItem.m_itemId, 70, __LINE__ );  // no scene execution due to the latch

    Json scene2Json = createEmptyScene;
    Json resetLatchBlock =
        replaceTokens( resetLatchThenBlock, { { "__SCENE_ID__", scene1Id }, { "__BLOCK_ID__", latchId } } );
    scene2Json[ "params" ][ "then" ].push_back( std::move( resetLatchBlock ) );

    const SceneID scene2Id = createScene( scene2Json );
    runSceneManually( scene2Id, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, scene2Json, scene2Id, "started" ); },
          [&]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, scene2Json, scene2Id, "finished" );
          } },
        __LINE__ );

    setItemValue( coredRemote, deviceWithItem.m_itemId, 10, __LINE__ );
    setItemValue( coredRemote, deviceWithItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "started", __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "finished", __LINE__ );
}

TEST_F( ResetLatchActionTest, resetLatch_invalidParams ) {  // 1.2
    Json sceneJson = createEmptyScene;
    Json resetLatchBlock = replaceTokens(
        resetLatchThenBlock, { { "__SCENE_ID__", "invalid_scene_id" }, { "__BLOCK_ID__", "invalid_block_id" } } );
    sceneJson[ "params" ][ "then" ].push_back( std::move( resetLatchBlock ) );
    sceneJson[ "exec_policy" ] = "check_result";

    const SceneID sceneId = createScene( sceneJson );

    runSceneManually( sceneId, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneJson, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneJson, sceneId, "failed" ); } },
        __LINE__ );
}

TEST_F( ResetLatchActionTest, resetSceneLatches_validCase ) {  // 2.1
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    auto deviceWithItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    std::string latchBlock = createLatch( createCompareNumbers( deviceWithItem.m_itemId ) );
    Json scene1Json = createSceneJson( latchBlock + ", " + latchBlock + ", " + latchBlock );
    const SceneID scene1Id = createScene( scene1Json, __LINE__ );

    setItemValue( coredRemote, deviceWithItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "started", __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "finished", __LINE__ );

    setItemValue( coredRemote, deviceWithItem.m_itemId, 10, __LINE__ );
    setItemValue( coredRemote, deviceWithItem.m_itemId, 70, __LINE__ );  // no scene execution due to the latch

    Json scene2Json = createEmptyScene;
    Json resetLatchBlock = replaceTokens( resetSceneLatchesThenBlock, { { "__SCENE_ID__", scene1Id } } );
    scene2Json[ "params" ][ "then" ].push_back( std::move( resetLatchBlock ) );

    const SceneID scene2Id = createScene( scene2Json );
    runSceneManually( scene2Id, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, scene2Json, scene2Id, "started" ); },
          [&]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, scene2Json, scene2Id, "finished" );
          } },
        __LINE__ );

    setItemValue( coredRemote, deviceWithItem.m_itemId, 10, __LINE__ );
    setItemValue( coredRemote, deviceWithItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "started", __LINE__ );
    testRunProgressBroadcast( scene1Json, scene1Id, "finished", __LINE__ );
}

TEST_F( ResetLatchActionTest, resetSceneLatches_invalidParams ) {  // 2.2
    Json sceneJson = createEmptyScene;
    Json resetLatchBlock = replaceTokens( resetSceneLatchesThenBlock, { { "__SCENE_ID__", "invalid_scene_id" } } );
    sceneJson[ "params" ][ "then" ].push_back( std::move( resetLatchBlock ) );
    sceneJson[ "exec_policy" ] = "check_result";

    const SceneID sceneId = createScene( sceneJson );

    runSceneManually( sceneId, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneJson, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneJson, sceneId, "failed" ); } },
        __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
