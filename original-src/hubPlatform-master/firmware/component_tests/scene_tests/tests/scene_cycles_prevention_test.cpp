#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

using SN = Scene number N

DONE    1. Detection of cycles in a scene graph with usage of "isSceneState"
DONE        1.1 Cases WITHOUT cycles in the scene graph (checking that there is no false positives in cycle detection)
DONE            1.1.1 Sequence of 3 scenes: S1 --isSceneState-> S2 --isSceneState-> S3
DONE            1.1.2 Parent and 2 children
                    S1 --isSceneState-> S2
                       --isSceneState-> S3
DONE        1.2 Cases WITH cycles in the scene graph
DONE            1.2.1 Cycle with 2 scenes: S1 --isSceneState-> S2 --isSceneState-> S1
DONE            1.2.2 Cycle with 3 scenes: S1 --isSceneState-> S2 --isSceneState-> S3 --isSceneState-> S1
DONE            1.2.3 Cycle with one of three children
                    S1 --isSceneState-> S2
                       --isSceneState-> S3
                       --isSceneState-> S4 --isSceneState-> S1
DONE            1.2.4 Cycle with one of the child's children
                    S1 --isSceneState-> S2 --isSceneState-> S4
                                           --isSceneState-> S5
                                           --isSceneState-> S6 --isSceneState-> S1
                       --isSceneState-> S3

DONE    2. Detection of cycles in a scene graph with usage of "isGroupState"
DONE        2.1 Cases WITHOUT cycles in the scene graph (checking that there is no false positives in cycle detection)
DONE            2.1.1 Sequence of 3 scenes: S1 --isGroupState-> S2 --isGroupState-> S3
DONE            2.1.2 Parent and 2 children
                    S1 --isGroupState-> S2
                       --isGroupState-> S3
DONE        2.2 Cases WITH cycles in the scene graph
DONE            2.2.1 Cycle with 2 scenes: S1 --isGroupState-> S2 --isGroupState-> S1
DONE            2.2.2 Cycle with 3 scenes: S1 --isGroupState-> S2 --isGroupState-> S3 --isGroupState-> S1
DONE            2.2.3 Cycle with one of three children
                    S1 --isGroupState-> S2
                       --isGroupState-> S3
                       --isGroupState-> S4 --isGroupState-> S1
DONE            2.2.4 Cycle with one of the child's children
                    S1 --isGroupState-> S2 --isGroupState-> S4
                                           --isGroupState-> S5
                                           --isGroupState-> S6 --isGroupState-> S1
                       --isGroupState-> S3

DONE    3. Detection of cycles in a scene graph with mix of "isGroupState" and "isSceneState"
DONE        3.1 Cases WITHOUT cycles in the scene graph (checking that there is no false positives in cycle detection)
DONE            3.1.1 Sequence of 3 scenes case 1: S1 --isGroupState-> S2 --isSceneState-> S3
DONE            3.1.2 Sequence of 3 scenes case 2: S1 --isSceneState-> S2 --isGroupState-> S3
DONE        3.2 Cases WITH cycles in the scene graph
DONE            3.2.1 Cycle with 2 scenes: S1 --isSceneState-> S2 --isGroupState-> S1
DONE            3.2.2 Cycle with 3 scenes case 1: S1 --isSceneState-> S2 --isGroupState-> S3 --isGroupState-> S1
DONE            3.2.3 Cycle with 3 scenes case 2: S1 --isSceneState-> S2 --isGroupState-> S3 --isSceneState-> S1
DONE            3.2.4 Cycle with one of three children
                    S1 --isSceneState-> S2
                       --isSceneState-> S3
                       --isSceneState-> S4 --isGroupState-> S1
DONE            3.2.5 Cycle with one of the child's children
                    S1 --isSceneState-> S2 --isGroupState-> S4
                                           --isGroupState-> S5
                                           --isGroupState-> S6 --isSceneState-> S1
                       --isSceneState-> S3

DONE    4. Detection of cycles in a scene graph with "runScene" action
DONE        4.1 Cases WITHOUT cycles in the scene graph (checking that there is no false positives in cycle detection)
DONE            4.1.1 Sequence of 3 scenes: S3 --runScene-> S2 --runScene-> S1
DONE            4.1.2 Parent and 2 children
                    S3 --runScene-> S1
                       --runScene-> S2
DONE        4.2 Cases WITH cycles in the scene graph
DONE            4.2.1 Cycle with 2 scenes: S1 --runScene-> S2 --runScene-> S1
DONE            4.2.2 Cycle with 3 scenes: S1 --runScene-> S3 --runScene-> S2 --runScene-> S1
DONE            4.2.3 Cycle with one of three children
                    S4 --runScene-> S1 --runScene-> S4
                       --runScene-> S2
                       --runScene-> S3
DONE            4.2.4 Cycle with one of the child's children
                    S6 --runScene-> S4 --runScene-> S1
                                       --runScene-> S2
                                       --runScene-> S3 --runScene-> S6
                       --runScene-> S5

DONE    5. Cycles created by mixing "runScene" action with "isGroupState" & "isSceneState" events
DONE        5.1 Cycle with "isSceneState": S1 --isSceneState-> S2 --runScene-> S1
DONE        5.2 Mix of all operations: S1 --isSceneState-> S2 --isGroupState-> S3 --runScene-> S1
DONE        5.3 Cycle with one of three children
                S1 --isSceneState-> S2
                   --isSceneState-> S3
                   --isSceneState-> S4 --runScene-> S1
DONE        5.4 Complicated mix of all operations:
                S1 --isSceneState-> S2 --isGroupState-> S4
                                       --isGroupState-> S5
                                       --isGroupState-> S6 --runScene-> S7 --isSceneState-> S8 --runScene-> S1
                   --isSceneState-> S3

DONE    6. Cycles are not detected for exception blocks
DONE        6.1 Cycle with 3 scenes: S1 --isSceneState-> S2 --isSceneState-> S1
                                                         S3 --isSceneState-> S1
DONE        6.2 EFWR-3664 (infinite loop on cycles detection)

*/

class SceneGraphCyclesPreventionTests : public SceneManagerTestsFixture {
public:
    using EventId = std::string;
    using EventsIds = std::vector< EventId >;
    using ScenesConnections = std::vector< std::pair< SceneID, std::string /* methodName */ > >;

    struct SceneInfo {
        SceneInfo( SceneID sceneId, EventsIds eventsIds )
        : m_sceneId( std::move( sceneId ) )
        , m_eventsIds( std::move( eventsIds ) ) {}

        SceneID m_sceneId;
        EventsIds m_eventsIds;
    };

public:
    void SetUp() override;

    SceneInfo createSceneExtended( const Json& request, int codeLine = -1 );
    void sendRequestAndCheckSuccess( const JsonObject& request, int codeLine = -1 ) const;

    static std::string buildExpectedReasonMessage( const SceneID& sourceSceneId, const ScenesConnections& connections );

    static JsonObject createSceneJsonWithRunSceneAction( const SceneID& executedSceneId );
    static JsonObject createSceneJsonWithIsSceneState( const SceneID& triggerSceneId );
    static JsonObject createSceneJsonWithIsGroupState( const SceneID& triggerSceneId, const EventId& groupId );
    static JsonObject createSceneJson( std::string whenBlockStr,
                                       std::string thenBlockStr,
                                       std::string exceptionBlockStr );

    static JsonObject createEditSceneJsonWithRunSceneAction( const SceneID& editableSceneId,
                                                             const SceneID& executedSceneId );
    static JsonObject createEditSceneJsonWithIsSceneState( const SceneID& editableSceneId,
                                                           const SceneID& triggerSceneId );
    static JsonObject createEditSceneJsonWithIsGroupState( const SceneID& editableSceneId,
                                                           const SceneID& triggerSceneId,
                                                           const EventId& groupId );
    static JsonObject createEditSceneJson( SceneID editableSceneId,
                                           std::string whenBlockStr,
                                           std::string thenBlockStr );

    std::string createItemStateBlock() const;
    static std::string createRunSceneThenBlock( const SceneID& executedSceneId );
    static std::string createSceneStateWhenBlock( const SceneID& triggerSceneId );
    static std::string createGroupStateWhenBlock( const SceneID& triggerSceneId, const EventId& groupId );

private:
    DeviceWithOneItemData m_testDeviceData;
};

void SceneGraphCyclesPreventionTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );

    m_testDeviceData = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int );
}

SceneGraphCyclesPreventionTests::SceneInfo SceneGraphCyclesPreventionTests::createSceneExtended( const Json& request,
                                                                                                 int codeLine ) {
    Json response;
    SceneID sceneId = createScene( request, response, codeLine );

    const Json& sceneWhenBlock = response[ "when" ];

    EventsIds eventsIds;
    eventsIds.reserve( sceneWhenBlock.size() );
    for( const Json& event : sceneWhenBlock ) {
        eventsIds.push_back( event[ "_id" ] );
    }

    return SceneInfo( std::move( sceneId ), std::move( eventsIds ) );
}

void SceneGraphCyclesPreventionTests::sendRequestAndCheckSuccess( const JsonObject& request, int codeLine ) const {
    const JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    Json nullJson;
    EXPECT_EQ( answer[ "error" ].dump(), nullJson.dump() ) << getCodeLineString( codeLine );
}

std::string SceneGraphCyclesPreventionTests::buildExpectedReasonMessage( const SceneID& sourceSceneId,
                                                                         const ScenesConnections& connections ) {
    std::stringstream message;
    if( !connections.empty() ) {
        message << "Detected cycle with following scenes: ";
        message << sourceSceneId;
        for( const auto& connection : connections ) {
            message << " --" << connection.second << "-> " << connection.first;
        }
    }
    return message.str();
}

JsonObject SceneGraphCyclesPreventionTests::createSceneJsonWithRunSceneAction( const SceneID& executedSceneId ) {
    return createSceneJson( {}, createRunSceneThenBlock( executedSceneId ), {} );
}

JsonObject SceneGraphCyclesPreventionTests::createEditSceneJsonWithRunSceneAction( const SceneID& editableSceneId,
                                                                                   const SceneID& executedSceneId ) {
    return createEditSceneJson( editableSceneId, {}, createRunSceneThenBlock( executedSceneId ) );
}

JsonObject SceneGraphCyclesPreventionTests::createSceneJsonWithIsSceneState( const SceneID& triggerSceneId ) {
    return createSceneJson( createSceneStateWhenBlock( triggerSceneId ), {}, {} );
}

JsonObject SceneGraphCyclesPreventionTests::createEditSceneJsonWithIsSceneState( const SceneID& editableSceneId,
                                                                                 const SceneID& triggerSceneId ) {
    return createEditSceneJson( editableSceneId, createSceneStateWhenBlock( triggerSceneId ), {} );
}

JsonObject SceneGraphCyclesPreventionTests::createSceneJsonWithIsGroupState( const SceneID& triggerSceneId,
                                                                             const EventId& groupId ) {
    return createSceneJson( createGroupStateWhenBlock( triggerSceneId, groupId ), {}, {} );
}

JsonObject SceneGraphCyclesPreventionTests::createEditSceneJsonWithIsGroupState( const SceneID& editableSceneId,
                                                                                 const SceneID& triggerSceneId,
                                                                                 const EventId& groupId ) {
    return createEditSceneJson( editableSceneId, createGroupStateWhenBlock( triggerSceneId, groupId ), {} );
}

std::string SceneGraphCyclesPreventionTests::createItemStateBlock() const {
    const KeyStringList requestReplaceList = {
        { "__ITEM_ID__", m_testDeviceData.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" }
    };
    return replaceTokensInString( isItemStateWhenBlock, requestReplaceList );
}

std::string SceneGraphCyclesPreventionTests::createRunSceneThenBlock( const SceneID& executedSceneId ) {
    return replaceTokensInString( runSceneThenBlock, { { "__SCENE_ID__", executedSceneId } } );
}

std::string SceneGraphCyclesPreventionTests::createSceneStateWhenBlock( const SceneID& triggerSceneId ) {
    const KeyStringList replaceList = { { "__SCENE_ID__", triggerSceneId }, { "__STATE__", "finished" } };
    return replaceTokensInString( isSceneStateWhenBlock, replaceList );
}

std::string SceneGraphCyclesPreventionTests::createGroupStateWhenBlock( const SceneID& triggerSceneId,
                                                                        const EventId& groupId ) {
    const KeyStringList replaceList = {
        { "__SCENE__", triggerSceneId }, { "__GROUP__", groupId }, { "__STATE__", "true" }
    };
    return replaceTokensInString( isGroupStateWhenBlock, replaceList );
}

JsonObject SceneGraphCyclesPreventionTests::createSceneJson( std::string whenBlockStr,
                                                             std::string thenBlockStr,
                                                             std::string exceptionBlockStr ) {
    const KeyStringList requestReplaceList = { { "__NAME__", "name" },
                                               { "__WHEN_BLOCKS__", std::move( whenBlockStr ) },
                                               { "__THEN_BLOCKS__", std::move( thenBlockStr ) },
                                               { "__EXCEPTION_BLOCKS__", std::move( exceptionBlockStr ) } };
    return replaceTokens( createSceneWithExceptionsRequest, requestReplaceList );
}

JsonObject SceneGraphCyclesPreventionTests::createEditSceneJson( SceneID editableSceneId,
                                                                 std::string whenBlockStr,
                                                                 std::string thenBlockStr ) {
    const KeyStringList requestReplaceList = { { "__ID__", std::move( editableSceneId ) },
                                               { "__NAME__", "name" },
                                               { "__WHEN_BLOCKS__", std::move( whenBlockStr ) },
                                               { "__THEN_BLOCKS__", std::move( thenBlockStr ) } };
    return replaceTokens( editScenesRequest, requestReplaceList );
}

TEST_F( SceneGraphCyclesPreventionTests, isSceneState_sequenceOfThreeScenesWithoutCycles ) {  // 1.1.1
    //  S1 --isSceneState-> S2 --isSceneState-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene2Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isSceneState_parentAndTwoChildrenWithoutCycles ) {  // 1.1.2
    //  S1 --isSceneState-> S2
    //     --isSceneState-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isSceneState_cycleWithTwoScenes ) {  // 1.2.1
    //  S1 --isSceneState-> S2 --isSceneState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithIsSceneState( scene1Id, scene2Id );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene2Id, "isSceneState" }, { scene1Id, "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isSceneState_cycleWithThreeScenes ) {  // 1.2.2
    //  S1 --isSceneState-> S2 --isSceneState-> S3 --isSceneState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene2Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithIsSceneState( scene1Id, scene3Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isSceneState" }, { scene3Id, "isSceneState" }, { scene1Id, "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isSceneState_cycleWithOneOfThreeChildren ) {  // 1.2.3
    //  S1 --isSceneState-> S2
    //     --isSceneState-> S3
    //     --isSceneState-> S4 --isSceneState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene4Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithIsSceneState( scene1Id, scene4Id );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene4Id, "isSceneState" }, { scene1Id, "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isSceneState_cycleWithChildsChildren ) {  // 1.2.4
    //  S1 --isSceneState-> S2 --isSceneState-> S4
    //                         --isSceneState-> S5
    //                         --isSceneState-> S6 --isSceneState-> S1
    //     --isSceneState-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );

    const SceneID scene4Id = createScene( createSceneJsonWithIsSceneState( scene2Id ), __LINE__ );
    const SceneID scene5Id = createScene( createSceneJsonWithIsSceneState( scene2Id ), __LINE__ );
    const SceneID scene6Id = createScene( createSceneJsonWithIsSceneState( scene2Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithIsSceneState( scene1Id, scene6Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isSceneState" }, { scene6Id, "isSceneState" }, { scene1Id, "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isGroupState_threeStatesWithoutCycles ) {  // 2.1.1
    //  S1 --isGroupState-> S2 --isGroupState-> S3
    const JsonObject scene1Json = createSceneJson( createItemStateBlock(), {}, {} );
    const SceneInfo scene1Info = createSceneExtended( scene1Json, __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const SceneID scene1Id = scene1Info.m_sceneId;
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene1Info.m_sceneId;
    const EventId event2Id = scene1Info.m_eventsIds.front();

    const SceneID scene3Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isGroupState_parentAndTwoChildrenWithoutCycles ) {  // 2.1.2
    //  S1 --isGroupState-> S2
    //     --isGroupState-> S3
    const JsonObject scene1Json = createSceneJson( createItemStateBlock(), {}, {} );
    const SceneInfo scene1Info = createSceneExtended( scene1Json, __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const SceneID scene1Id = scene1Info.m_sceneId;
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const SceneID scene2Id = createScene( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isGroupState_cycleWithTwoScenes ) {  // 2.2.1
    //  S1 --isGroupState-> S2 --isGroupState-> S1
    const std::string itemStateBlockStr = createItemStateBlock();

    const SceneInfo scene1Info = createSceneExtended( createSceneJson( itemStateBlockStr, {}, {} ), __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const SceneID scene1Id = scene1Info.m_sceneId;
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const Json scene2Json = createSceneJsonWithIsGroupState( scene1Id, event1Id );
    const SceneInfo scene2Info = createSceneExtended( scene2Json, __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();

    JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene2Id, event2Id );
    JsonObject itemStateBlockJson = JsonObject::parse( itemStateBlockStr );
    itemStateBlockJson[ "_id" ] = event1Id;
    editScene1Json[ "params" ][ "eo" ][ "when" ].push_back( std::move( itemStateBlockJson ) );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene2Id, "isGroupState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isGroupState_cycleWithThreeScenes ) {  // 2.2.2
    //  S1 --isGroupState-> S2 --isGroupState-> S3 --isGroupState-> S1
    const std::string itemStateBlockStr = createItemStateBlock();

    const SceneInfo scene1Info = createSceneExtended( createSceneJson( itemStateBlockStr, {}, {} ), __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const SceneID scene1Id = scene1Info.m_sceneId;
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();

    const SceneInfo scene3Info = createSceneExtended( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    ASSERT_FALSE( scene3Info.m_eventsIds.empty() );
    const SceneID scene3Id = scene3Info.m_sceneId;
    const EventId event3Id = scene3Info.m_eventsIds.front();

    JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene3Id, event3Id );
    JsonObject itemStateBlockJson = JsonObject::parse( itemStateBlockStr );
    itemStateBlockJson[ "_id" ] = event1Id;
    editScene1Json[ "params" ][ "eo" ][ "when" ].push_back( itemStateBlockJson );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isGroupState" }, { scene3Id, "isGroupState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isGroupState_cycleWithOneOfThreeChildren ) {  // 2.2.4
    //  S1 --isGroupState-> S2
    //     --isGroupState-> S3
    //     --isGroupState-> S4 --isGroupState-> S1
    const std::string itemStateBlockStr = createItemStateBlock();

    const SceneInfo scene1Info = createSceneExtended( createSceneJson( itemStateBlockStr, {}, {} ), __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const SceneID scene1Id = scene1Info.m_sceneId;
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const SceneID scene2Id = createScene( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    const SceneInfo scene4Info = createSceneExtended( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    ASSERT_FALSE( scene4Info.m_eventsIds.empty() );
    const SceneID scene4Id = scene4Info.m_sceneId;
    const EventId event4Id = scene4Info.m_eventsIds.front();

    JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene4Id, event4Id );
    JsonObject itemStateBlockJson = JsonObject::parse( itemStateBlockStr );
    itemStateBlockJson[ "_id" ] = event1Id;
    editScene1Json[ "params" ][ "eo" ][ "when" ].push_back( std::move( itemStateBlockJson ) );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene4Id, "isGroupState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, isGroupState_cycleWithChildsChildren ) {  // 2.2.5
    //  S1 --isGroupState-> S2 --isGroupState-> S4
    //                         --isGroupState-> S5
    //                         --isGroupState-> S6 --isGroupState-> S1
    //     --isGroupState-> S3
    const std::string itemStateBlockStr = createItemStateBlock();

    const SceneInfo scene1Info = createSceneExtended( createSceneJson( itemStateBlockStr, {}, {} ), __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const SceneID scene1Id = scene1Info.m_sceneId;
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();
    const SceneID scene3Id = createScene( createSceneJsonWithIsGroupState( scene1Id, event1Id ), __LINE__ );

    const SceneID scene4Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    const SceneID scene5Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    const SceneInfo scene6Info = createSceneExtended( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    ASSERT_FALSE( scene6Info.m_eventsIds.empty() );
    const SceneID scene6Id = scene6Info.m_sceneId;
    const EventId event6Id = scene6Info.m_eventsIds.front();

    JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene6Id, event6Id );
    JsonObject itemStateBlockJson = JsonObject::parse( itemStateBlockStr );
    itemStateBlockJson[ "_id" ] = event1Id;
    editScene1Json[ "params" ][ "eo" ][ "when" ].push_back( std::move( itemStateBlockJson ) );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isGroupState" }, { scene6Id, "isGroupState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_threeStatesWithoutCyclesCase1 ) {  // 3.1.1
    //  S1 --isGroupState-> S2 --isSceneState-> S3
    const JsonObject scene1Json = createSceneJson( createItemStateBlock(), {}, {} );
    const SceneInfo scene1Info = createSceneExtended( scene1Json, __LINE__ );
    ASSERT_FALSE( scene1Info.m_eventsIds.empty() );
    const EventId event1Id = scene1Info.m_eventsIds.front();

    const SceneID scene2Id = createScene( createSceneJsonWithIsGroupState( scene1Info.m_sceneId, event1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene2Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_threeStatesWithoutCyclesCase2 ) {  // 3.1.2
    //  S1 --isSceneState-> S2 --isGroupState-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );

    const EventId event2Id = scene2Info.m_eventsIds.front();
    const SceneID scene3Id = createScene( createSceneJsonWithIsGroupState( scene2Info.m_sceneId, event2Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_cycleWithTwoScenes ) {  // 3.2.1
    //  S1 --isSceneState-> S2 --isGroupState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();

    const JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene2Id, event2Id );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene2Id, "isSceneState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_cycleWithThreeScenes1 ) {  // 3.2.2
    //  S1 --isSceneState-> S2 --isGroupState-> S3 --isGroupState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();

    const SceneInfo scene3Info = createSceneExtended( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    ASSERT_FALSE( scene3Info.m_eventsIds.empty() );
    const SceneID scene3Id = scene3Info.m_sceneId;
    const EventId event3Id = scene3Info.m_eventsIds.front();

    const JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene3Id, event3Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isSceneState" }, { scene3Id, "isGroupState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_cycleWithThreeScenes2 ) {  // 3.2.3
    //  S1 --isSceneState-> S2 --isGroupState-> S3 --isSceneState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();

    const SceneID scene3Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithIsSceneState( scene1Id, scene3Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isSceneState" }, { scene3Id, "isGroupState" }, { scene1Id, "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_cycleWithOneOfThreeChildren ) {  // 3.2.4
    //  S1 --isSceneState-> S2
    //     --isSceneState-> S3
    //     --isSceneState-> S4 --isGroupState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneInfo scene4Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene4Info.m_eventsIds.empty() );
    const SceneID scene4Id = scene4Info.m_sceneId;
    const EventId event4Id = scene4Info.m_eventsIds.front();

    const JsonObject editScene1Json = createEditSceneJsonWithIsGroupState( scene1Id, scene4Id, event4Id );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene4Id, "isSceneState" }, { scene1Id, "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, groupSceneStates_cycleWithChildsChildren ) {  // 3.2.5
    //  S1 --isSceneState-> S2 --isGroupState-> S4
    //                         --isGroupState-> S5
    //                         --isGroupState-> S6 --isSceneState-> S1
    //     --isSceneState-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );

    const SceneID scene4Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    const SceneID scene5Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    const SceneID scene6Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithIsSceneState( scene1Id, scene6Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene2Id, "isSceneState" }, { scene6Id, "isGroupState" }, { scene1Id, "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, runScene_sequenceOfThreeScenesWithoutCycles ) {  // 4.1.1
    //  S3 --runScene-> S2 --runScene-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithRunSceneAction( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithRunSceneAction( scene2Id ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, runScene_parentAndTwoChildrenWithoutCycles ) {  // 4.1.2
    //  S3 --runScene-> S1
    //     --runScene-> S2

    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const std::string thenBlock1 = createRunSceneThenBlock( scene1Id );
    const std::string thenBlock2 = createRunSceneThenBlock( scene2Id );
    const SceneID scene3Id = createScene( createSceneJson( {}, thenBlock1 + ", " + thenBlock2, {} ), __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, runScene_cycleWithTwoScenes ) {  // 4.2.1
    //  S1 --runScene-> S2 --runScene-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithRunSceneAction( scene1Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithRunSceneAction( scene1Id, scene2Id );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene2Id, "runScene" }, { scene1Id, "runScene" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, runScene_cycleWithThreeScenes ) {  // 4.2.2
    //  S1 --runScene-> S3 --runScene-> S2 --runScene-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithRunSceneAction( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithRunSceneAction( scene2Id ), __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithRunSceneAction( scene1Id, scene3Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene1Id, { { scene3Id, "runScene" }, { scene2Id, "runScene" }, { scene1Id, "runScene" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, runScene_cycleWithOneOfThreeChildren ) {  // 4.2.3
    //  S4 --runScene-> S1 --runScene-> S4
    //     --runScene-> S2
    //     --runScene-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene3Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const std::string thenBlock1 = createRunSceneThenBlock( scene1Id );
    const std::string thenBlock2 = createRunSceneThenBlock( scene2Id );
    const std::string thenBlock3 = createRunSceneThenBlock( scene3Id );
    const Json scene4Json = createSceneJson( {}, thenBlock1 + ", " + thenBlock2 + ", " + thenBlock3, {} );
    const SceneID scene4Id = createScene( scene4Json, __LINE__ );

    const JsonObject editScene1Json = createEditSceneJsonWithRunSceneAction( scene1Id, scene4Id );
    const std::string expectedMessage =
        buildExpectedReasonMessage( scene1Id, { { scene4Id, "runScene" }, { scene1Id, "runScene" } } );
    sendRequestAndCheckFailedAnswer( editScene1Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, runScene_cycleWithChildsChildren ) {  // 4.2.4
    //  S6 --runScene-> S4 --runScene-> S1
    //                     --runScene-> S2
    //                     --runScene-> S3 --runScene-> S6
    //     --runScene-> S5
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene3Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const std::string thenBlock1 = createRunSceneThenBlock( scene1Id );
    const std::string thenBlock2 = createRunSceneThenBlock( scene2Id );
    const std::string thenBlock3 = createRunSceneThenBlock( scene3Id );
    const Json scene4Json = createSceneJson( {}, thenBlock1 + ", " + thenBlock2 + ", " + thenBlock3, {} );
    const SceneID scene4Id = createScene( scene4Json, __LINE__ );
    const SceneID scene5Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const std::string thenBlock4 = createRunSceneThenBlock( scene4Id );
    const std::string thenBlock5 = createRunSceneThenBlock( scene5Id );
    const SceneID scene6Id = createScene( createSceneJson( {}, thenBlock4 + ", " + thenBlock5, {} ), __LINE__ );

    const JsonObject editScene3Json = createEditSceneJsonWithRunSceneAction( scene3Id, scene6Id );
    const std::string expectedMessage = buildExpectedReasonMessage(
        scene3Id, { { scene6Id, "runScene" }, { scene4Id, "runScene" }, { scene3Id, "runScene" } } );
    sendRequestAndCheckFailedAnswer( editScene3Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, allOperationsMix_cycleUsingRunSceneAndIsSceneState ) {  // 5.1
    //  S1 --isSceneState-> S2 --runScene-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    std::string whenBlock2 = createSceneStateWhenBlock( scene1Id );
    std::string thenBlock2 = createRunSceneThenBlock( scene1Id );
    const Json scene2Json = createSceneJson( std::move( whenBlock2 ), std::move( thenBlock2 ), {} );
    const std::string expectedMessage =
        buildExpectedReasonMessage( "*new scene*", { { scene1Id, "runScene" }, { "*new scene*", "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( scene2Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, allOperationsMix_cycleWithAllOperations ) {  // 5.2
    //  S1 --isSceneState-> S2 --isGroupState-> S3 --runScene-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();

    std::string whenBlock3 = createGroupStateWhenBlock( scene2Id, event2Id );
    std::string thenBlock3 = createRunSceneThenBlock( scene1Id );
    const Json scene3Json = createSceneJson( std::move( whenBlock3 ), std::move( thenBlock3 ), {} );
    const std::string expectedMessage = buildExpectedReasonMessage(
        "*new scene*", { { scene1Id, "runScene" }, { scene2Id, "isSceneState" }, { "*new scene*", "isGroupState" } } );
    sendRequestAndCheckFailedAnswer( scene3Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, allOperationsMix_cycleWithOneOfThreeChildren ) {  // 5.3
    //  S1 --isSceneState-> S2
    //     --isSceneState-> S3
    //     --isSceneState-> S4 --runScene-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );

    std::string whenBlock4 = createSceneStateWhenBlock( scene1Id );
    std::string thenBlock4 = createRunSceneThenBlock( scene1Id );
    const Json scene4Json = createSceneJson( std::move( whenBlock4 ), std::move( thenBlock4 ), {} );
    const std::string expectedMessage =
        buildExpectedReasonMessage( "*new scene*", { { scene1Id, "runScene" }, { "*new scene*", "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( scene4Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, allOperationsMix_complicatedMixOfAllOperations ) {  // 5.4
    //  S1 --isSceneState-> S2 --isGroupState-> S4
    //                         --isGroupState-> S5
    //                         --isGroupState-> S6 --runScene-> S7 --isSceneState-> S8 --runScene-> S1
    //     --isSceneState-> S3
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const SceneInfo scene2Info = createSceneExtended( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );
    ASSERT_FALSE( scene2Info.m_eventsIds.empty() );
    const SceneID scene2Id = scene2Info.m_sceneId;
    const EventId event2Id = scene2Info.m_eventsIds.front();
    const SceneID scene3Id = createScene( createSceneJsonWithIsSceneState( scene1Id ), __LINE__ );

    const SceneID scene4Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );
    const SceneID scene5Id = createScene( createSceneJsonWithIsGroupState( scene2Id, event2Id ), __LINE__ );

    const std::string whenBlock6 = createGroupStateWhenBlock( scene2Id, event2Id );
    const std::string thenBlock6 = createRunSceneThenBlock( scene4Id );  // will be edited after creation of S7
    const SceneID scene6Id = createScene( createSceneJson( whenBlock6, thenBlock6, {} ), __LINE__ );

    const SceneID scene7Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const std::string thenBlock6Edit = createRunSceneThenBlock( scene7Id );
    const JsonObject editScene6Json = createEditSceneJson( scene6Id, whenBlock6, thenBlock6Edit );
    sendRequestAndCheckSuccess( editScene6Json, __LINE__ );
    const JsonObject broadcast = waitForBroadcast( "hub.scene.changed", __FILE__, __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );

    std::string whenBlock8 = createSceneStateWhenBlock( scene7Id );
    std::string thenBlock8 = createRunSceneThenBlock( scene1Id );
    const Json scene8Json = createSceneJson( std::move( whenBlock8 ), std::move( thenBlock8 ), {} );
    std::string expectedMessage = buildExpectedReasonMessage( "*new scene*",
                                                              { { scene1Id, "runScene" },
                                                                { scene2Id, "isSceneState" },
                                                                { scene6Id, "isGroupState" },
                                                                { scene7Id, "runScene" },
                                                                { "*new scene*", "isSceneState" } } );
    sendRequestAndCheckFailedAnswer( scene8Json, Errors::SCENE_CYCLE_DETECTED, expectedMessage, __LINE__ );
}

TEST_F( SceneGraphCyclesPreventionTests, exceptions_cycleNotDetectedForExceptionBlocks ) {  // 6.1
    //  S1 --isSceneState-> S2 --isSceneState-> S1
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const Json scene2Json = createSceneJson( {}, {}, createSceneStateWhenBlock( scene1Id ) );
    const SceneID scene2Id = createScene( scene2Json, __LINE__ );

    const Json editScene1Json = createEditSceneJson( scene1Id, {}, createSceneStateWhenBlock( scene2Id ) );
    sendRequestAndCheckAnswer( editScene1Json, __LINE__ );
    const Json broadcast = waitForBroadcast( "hub.scene.changed", __FILE__, __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << broadcast;
}

TEST_F( SceneGraphCyclesPreventionTests, exceptions_EFWR3664 ) {  // 6.2
    const SceneID scene1Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene2Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );
    const SceneID scene3Id = createScene( createEmptyWhenBlockSceneJson, __LINE__ );

    const JsonObject scene4Json = replaceTokens(
        EFWR_3664_createScene,
        { { "__SCENE_ID_1__", scene1Id }, { "__SCENE_ID_2__", scene2Id }, { "__SCENE_ID_3__", scene3Id } } );
    const SceneID scene4Id = createScene( scene4Json, __LINE__ );

    const SceneID scene5Id = createScene( createSceneJsonWithRunSceneAction( scene4Id ), __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
