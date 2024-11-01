#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "scenes_manager/test_remote_interface/test_scenes_remote_interface.h"
#include "base_utils/gregorian_calendar.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1. AND parent
DONE        1.1 compare numbers events referencing the same item with
DONE        1.2 nested compare numbers events referencing the same item
DONE        1.3 date events on same time
DONE        1.4 nested date events on same time
DONE    2. OR parent
DONE        2.1 compare numbers events referencing the same item

*/

const std::chrono::seconds NoBroadcastsCheckPeriod{ 2 };

class TransactionalEventsHandlingTests : public SceneManagerTestsFixture {
public:
    void SetUp() override;

    std::string createIntItem();

    static JsonObject buildSceneWithNestedWhenBlocks( const Json& whenBlock1, const Json& whenBlock2 );

    static JsonObject buildOrOperator( const std::vector< JsonObject >& blocks );
    static JsonObject buildAndOperator( const std::vector< JsonObject >& blocks );
    static JsonObject buildOperatorWithBlocks( const std::string& base, const std::vector< JsonObject >& blocks );

    static JsonObject buildCompareNumbersWhenBlock( const std::string& sourceType,
                                                    const std::string& sourceId,
                                                    const std::string& comparator,
                                                    int value );
};

void TransactionalEventsHandlingTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

std::string TransactionalEventsHandlingTests::createIntItem() {
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    return device.m_itemId;
}

JsonObject TransactionalEventsHandlingTests::buildSceneWithNestedWhenBlocks( const Json& whenBlock1,
                                                                             const Json& whenBlock2 ) {
    // Scene: AND( OR( block1 ) ), NOT( NOT( block2 ) )
    const Json nestedWhenBlock1 = [&whenBlock1]() {
        const Json oredBlock = buildOrOperator( { whenBlock1 } );
        return buildAndOperator( { oredBlock } );
    }();
    const Json nestedWhenBlock2 = [&whenBlock2]() {
        const Json inversedBlock = replaceTokens( notOperatorWhenBlock, { { "__BLOCK__", whenBlock2.dump() } } );
        return replaceTokens( notOperatorWhenBlock, { { "__BLOCK__", inversedBlock.dump() } } );
    }();

    const Json parent = buildAndOperator( { nestedWhenBlock1, nestedWhenBlock2 } );

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( parent );
    return scene;
}

JsonObject TransactionalEventsHandlingTests::buildOrOperator( const std::vector< JsonObject >& blocks ) {
    return buildOperatorWithBlocks( orOperatorWhenBlock, blocks );
}

JsonObject TransactionalEventsHandlingTests::buildAndOperator( const std::vector< JsonObject >& blocks ) {
    return buildOperatorWithBlocks( andOperatorWhenBlock, blocks );
}

JsonObject TransactionalEventsHandlingTests::buildOperatorWithBlocks( const std::string& base,
                                                                      const std::vector< JsonObject >& blocks ) {
    std::string blocksStr;
    for( size_t i = 0; i < blocks.size(); ++i ) {
        if( i != 0 )
            blocksStr += ", ";

        blocksStr += blocks[ i ].dump();
    }

    return replaceTokens( base, { { "__BLOCKS__", blocksStr } } );
}

JsonObject TransactionalEventsHandlingTests::buildCompareNumbersWhenBlock( const std::string& sourceType,
                                                                           const std::string& sourceId,
                                                                           const std::string& comparator,
                                                                           int value ) {
    return replaceTokens( compareNumbersWhenBlock,
                          { { "__SOURCE_NAME__", sourceType },
                            { "__SOURCE_TYPE__", sourceType },
                            { "__SOURCE_VALUE__", '\"' + sourceId + '\"' },
                            { "__COMPARATOR__", comparator },
                            { "__TYPE__", "int" },
                            { "__VALUE__", std::to_string( value ) } } );
}

TEST_F( TransactionalEventsHandlingTests, eventsReferencingSameItemWithAndParent ) {  // 1.1
    // AND( >60, <=50 ), before transactional handling this would lead to scene execution
    const std::string testItemId = createIntItem();
    const Json whenBlock1 = buildCompareNumbersWhenBlock( "item", testItemId, ">", 60 );
    const Json whenBlock2 = [&testItemId]() {
        const Json block = buildCompareNumbersWhenBlock( "item", testItemId, ">", 50 );
        return replaceTokens( notOperatorWhenBlock, { { "__BLOCK__", block.dump() } } );
    }();
    const Json parent = buildAndOperator( { whenBlock1, whenBlock2 } );

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( parent );
    const SceneID sceneId = createScene( scene );

    setItemValue( makeRemote(), testItemId, Json( 70 ), __LINE__ );
    checkNoBroadcastReceived( "No scene execution expected", __LINE__, NoBroadcastsCheckPeriod );
}

TEST_F( TransactionalEventsHandlingTests, nestedEventsReferencingSameItemWithAndParent ) {  // 1.2
    // AND( >60, <50 ), before transactional handling this would lead to scene execution
    const std::string testItemId = createIntItem();
    const Json whenBlock1 = buildCompareNumbersWhenBlock( "item", testItemId, ">", 60 );
    const Json whenBlock2 = buildCompareNumbersWhenBlock( "item", testItemId, "<", 50 );
    const Json scene = buildSceneWithNestedWhenBlocks( whenBlock1, whenBlock2 );
    const SceneID sceneId = createScene( scene );

    setItemValue( makeRemote(), testItemId, Json( 70 ), __LINE__ );
    checkNoBroadcastReceived( "No scene execution expected", __LINE__, NoBroadcastsCheckPeriod );
}

TEST_F( TransactionalEventsHandlingTests, dateEventsAtSameTimeWithAndParent ) {  // 1.3
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto resetDate = GregorianCalendar::Date( "2020-03-02T00:20:44+0000" ).toSeconds();
    auto runDate = GregorianCalendar::Date( "2020-03-18T15:54:59+0000" );
    scenesManagerInterface.setTestClock( resetDate );

    const KeyStringList timeArray = { { "__TIME_ARRAY__", "\"15:55\"" } };
    const Json dateEvent1 = replaceTokens( isDateDailyWhenBlock, timeArray );
    const Json dateEvent2 = replaceTokens( isDateDailyWhenBlock, timeArray );
    const Json parent = buildAndOperator( { dateEvent1, dateEvent2 } );

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( parent );
    const SceneID sceneId = createScene( scene );

    scenesManagerInterface.setTestClock( runDate.toSeconds() );
    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );
}

TEST_F( TransactionalEventsHandlingTests, nestedDateEventsWithAndParent ) {  // 1.4
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto resetDate = GregorianCalendar::Date( "2020-03-02T00:20:44+0000" ).toSeconds();
    auto runDate = GregorianCalendar::Date( "2020-03-18T15:54:59+0000" );
    scenesManagerInterface.setTestClock( resetDate );

    const KeyStringList timeArray = { { "__TIME_ARRAY__", "\"15:55\"" } };
    const Json dateEvent1 = replaceTokens( isDateDailyWhenBlock, timeArray );
    const Json dateEvent2 = replaceTokens( isDateDailyWhenBlock, timeArray );
    const Json scene = buildSceneWithNestedWhenBlocks( dateEvent1, dateEvent2 );
    const SceneID sceneId = createScene( scene );

    scenesManagerInterface.setTestClock( runDate.toSeconds() );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );
}

TEST_F( TransactionalEventsHandlingTests, eventsReferencingSameItemWithOrParent ) {  // 2.1
    const std::string testItemId = createIntItem();
    const Json whenBlock1 = buildCompareNumbersWhenBlock( "item", testItemId, ">", 50 );
    const Json whenBlock2 = buildCompareNumbersWhenBlock( "item", testItemId, ">", 50 );
    const Json parent = buildOrOperator( { whenBlock1, whenBlock2 } );

    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( parent );
    const SceneID sceneId = createScene( scene );

    setItemValue( makeRemote(), testItemId, Json( 60 ), __LINE__ );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

    setItemValue( makeRemote(), testItemId, Json( 40 ), __LINE__ );
    checkNoBroadcastReceived( "No scene execution expected", __LINE__, NoBroadcastsCheckPeriod );
}

}  // namespace Tests
}  // namespace HubPlatform
