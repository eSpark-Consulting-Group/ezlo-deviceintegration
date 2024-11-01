#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"

/**

TESTPLAN

 These test cases cover handling of action exceptions in regard of interacting with other scene mechanisms.
 Like execution policies, broadcasts, handling of changes in when blocks, etc.
 Detailed tests for particular exceptions types are implemented as unit tests.

DONE    1. Action executed, when exception satisfied (one action in scene)
DONE    2. Action not executed, when exceptions dissatisfied (one action in scene)
DONE    3. Exceptions doesn't affect other action
DONE        3.1 with "ignore_result" exec policy
DONE        3.2 with "check_result" exec policy
DONE        3.3 with "wait_result" exec policy
DONE    4. All actions blocked by exceptions
DONE    5. Exception state cannot be evaluated (referenced block or expression was deleted)
DONE        5.1 single action
DONE        5.2 multiple actions
DONE    6. Usage of expression

 */

namespace HubPlatform {
namespace Tests {

class ActionExceptionsComponentTest : public SceneManagerTestsFixture {
public:
    struct ActionWithExceptionData {
        ActionWithExceptionData( std::string itemId, std::string blockId, int setterValue, int exceptionValue )
        : itemId( std::move( itemId ) )
        , blockId( std::move( blockId ) )
        , setterValue( setterValue )
        , exceptionValue( exceptionValue ) {}

        std::string itemId;
        std::string blockId;
        int setterValue = 0;
        int exceptionValue = 0;
    };

    using SceneData = std::pair< SceneID, Json >;

public:
    void SetUp() override;

    void generateItems( int exceptionItemsCount, int codeLine );

    SceneData setupTestWithOneAction( const std::string& execPolicy, int codeLine );
    SceneData setupTestWithThreeActions( const std::string& execPolicy, int codeLine );

    void testExceptionDoesntAffectOtherActions( const std::string& executionPolicy );

    static Json createSceneJson( const std::vector< Json >& whenBlocks,
                                 const std::vector< ActionWithExceptionData >& actions );

    static Json blockReferenceExceptionJson( const std::string& sceneId, const std::string& blockId );
    static Json expressionReferenceExceptionJson( const std::string& expressionName );

    static Json isItemStateJson( std::string itemId, int value );
    static Json setItemValueJson( std::string itemId, int value, Json controlBlock );
    static Json createSceneJson( const std::vector< Json >& whenBlocks,
                                 const std::vector< Json >& thenBlocks,
                                 const std::vector< Json >& exceptionBlocks );

public:
    static constexpr int ExceptionValue1 = 1;
    static constexpr int ExceptionValue2 = 2;
    static constexpr int ExceptionValue3 = 3;
    static constexpr int SetterValue1 = 11;
    static constexpr int SetterValue2 = 12;
    static constexpr int SetterValue3 = 13;
    static constexpr int TriggerValue = 20;

    std::vector< std::string > m_exceptionItemIds;
    std::string m_triggerItemId;
};

void ActionExceptionsComponentTest::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void ActionExceptionsComponentTest::generateItems( int exceptionItemsCount, int codeLine ) {
    for( int i = 0; i < exceptionItemsCount; ++i ) {
        auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );
        m_exceptionItemIds.push_back( std::move( device.m_itemId ) );
    }

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );
    m_triggerItemId = std::move( device.m_itemId );
}

ActionExceptionsComponentTest::SceneData ActionExceptionsComponentTest::setupTestWithOneAction(
    const std::string& execPolicy,
    int codeLine ) {
    generateItems( 1, codeLine );

    const ActionWithExceptionData action{ m_exceptionItemIds[ 0 ], "${test_1}", SetterValue1, ExceptionValue1 };
    const Json whenBlock = isItemStateJson( m_triggerItemId, TriggerValue );

    Json sceneJson = createSceneJson( { whenBlock }, { action } );
    if( !execPolicy.empty() )
        sceneJson[ "exec_policy" ] = execPolicy;
    std::cout << sceneJson.dump();
    SceneID sceneId = createScene( sceneJson, codeLine );

    return std::make_pair( std::move( sceneId ), std::move( sceneJson ) );
}

ActionExceptionsComponentTest::SceneData ActionExceptionsComponentTest::setupTestWithThreeActions(
    const std::string& execPolicy,
    int codeLine ) {
    generateItems( 3, codeLine );

    const ActionWithExceptionData action1{ m_exceptionItemIds[ 0 ], "${test_1}", SetterValue1, ExceptionValue1 };
    const ActionWithExceptionData action2{ m_exceptionItemIds[ 1 ], "${test_2}", SetterValue2, ExceptionValue2 };
    const ActionWithExceptionData action3{ m_exceptionItemIds[ 2 ], "${test_3}", SetterValue3, ExceptionValue3 };
    const Json whenBlock = isItemStateJson( m_triggerItemId, TriggerValue );

    Json sceneJson = createSceneJson( { whenBlock }, { action1, action2, action3 } );
    if( !execPolicy.empty() )
        sceneJson[ "exec_policy" ] = execPolicy;
    SceneID sceneId = createScene( sceneJson, codeLine );

    return std::make_pair( std::move( sceneId ), std::move( sceneJson ) );
}

void ActionExceptionsComponentTest::testExceptionDoesntAffectOtherActions( const std::string& executionPolicy ) {
    const SceneData sceneData = setupTestWithThreeActions( executionPolicy, __LINE__ );
    const SceneID& sceneId = sceneData.first;
    const Json& sceneJson = sceneData.second;

    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    setItemValueWithoutCheck( coredRemote, m_exceptionItemIds[ 0 ], 47, __LINE__ );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 0 ], 47, __LINE__ ) );

    setItemValueWithoutCheck( coredRemote, m_exceptionItemIds[ 1 ], ExceptionValue2, __LINE__ );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 1 ], ExceptionValue2, __LINE__ ) );

    setItemValueWithoutCheck( coredRemote, m_exceptionItemIds[ 2 ], ExceptionValue3, __LINE__ );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 2 ], ExceptionValue3, __LINE__ ) );

    setItemValueWithoutCheck( coredRemote, m_triggerItemId, TriggerValue, __LINE__ );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_triggerItemId, TriggerValue, __LINE__ ) );

    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "finished", __LINE__ ) );

    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 1 ], SetterValue2, __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 2 ], SetterValue3, __LINE__ ) );
}

Json ActionExceptionsComponentTest::createSceneJson( const std::vector< Json >& whenBlocks,
                                                     const std::vector< ActionWithExceptionData >& actions ) {
    std::vector< Json > thenBlocks;
    std::vector< Json > exceptionBlocks;

    for( const auto& action : actions ) {
        Json control = blockReferenceExceptionJson( std::string(), action.blockId );
        Json thenBlock = setItemValueJson( action.itemId, action.setterValue, std::move( control ) );
        Json exceptionBlock = isItemStateJson( action.itemId, action.exceptionValue );
        exceptionBlock[ "_id" ] = action.blockId;

        thenBlocks.push_back( std::move( thenBlock ) );
        exceptionBlocks.push_back( std::move( exceptionBlock ) );
    }

    return createSceneJson( whenBlocks, thenBlocks, exceptionBlocks );
}

Json ActionExceptionsComponentTest::blockReferenceExceptionJson( const std::string& sceneId,
                                                                 const std::string& blockId ) {
    if( sceneId.empty() )
        return replaceTokens( blockReferenceExceptionBlock, { { "__BLOCK_ID__", blockId } } );

    return replaceTokens( externalBlockReferenceExceptionBlock,
                          { { "__SCENE_ID__", sceneId }, { "__BLOCK_ID__", blockId } } );
}

Json ActionExceptionsComponentTest::expressionReferenceExceptionJson( const std::string& expressionName ) {
    return replaceTokens( expressionReferenceExceptionBlock, { { "__NAME__", expressionName } } );
}

Json ActionExceptionsComponentTest::ActionExceptionsComponentTest::isItemStateJson( std::string itemId, int value ) {
    return replaceTokens(
        isItemStateWhenBlock,
        { { "__ITEM_ID__", std::move( itemId ) }, { "__TYPE__", "int" }, { "__VALUE__", std::to_string( value ) } } );
}

Json ActionExceptionsComponentTest::ActionExceptionsComponentTest::setItemValueJson( std::string itemId,
                                                                                     int value,
                                                                                     Json controlBlock ) {
    Json result = replaceTokens(
        setItemValueThenBlock,
        { { "__ITEM_ID__", std::move( itemId ) }, { "__TYPE__", "int" }, { "__VALUE__", std::to_string( value ) } } );

    if( !controlBlock.empty() )
        result[ "control" ] = std::move( controlBlock );

    return result;
}

Json ActionExceptionsComponentTest::createSceneJson( const std::vector< Json >& whenBlocks,
                                                     const std::vector< Json >& thenBlocks,
                                                     const std::vector< Json >& exceptionBlocks ) {
    Json result = replaceTokens( createSceneRequest,
                                 { { "__NAME__", "some_name" },
                                   { "__EXEC_POLICY__", "ignore_result" },
                                   { "__WHEN_BLOCKS__", "" },
                                   { "__THEN_BLOCKS__", "" } } );
    if( !whenBlocks.empty() )
        result[ "params" ][ "when" ] = whenBlocks;
    if( !whenBlocks.empty() )
        result[ "params" ][ "then" ] = thenBlocks;
    if( !exceptionBlocks.empty() )
        result[ "params" ][ "exceptions" ] = exceptionBlocks;

    return result;
}

TEST_F( ActionExceptionsComponentTest, exceptionSatisfied ) {  // 1.
    const SceneData sceneData = setupTestWithOneAction( "", __LINE__ );
    const SceneID& sceneId = sceneData.first;
    const Json& sceneJson = sceneData.second;

    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    setItemValueWithoutCheck( coredRemote, m_exceptionItemIds[ 0 ], ExceptionValue1, __LINE__ );
    setItemValueWithoutCheck( coredRemote, m_triggerItemId, TriggerValue, __LINE__ );

    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 0 ], ExceptionValue1, __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_triggerItemId, TriggerValue, __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 0 ], SetterValue1, __LINE__ ) );
}

TEST_F( ActionExceptionsComponentTest, exceptionDissatisfied ) {  // 2.
    const SceneData sceneData = setupTestWithOneAction( "", __LINE__ );
    const SceneID& sceneId = sceneData.first;
    const Json& sceneJson = sceneData.second;

    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    setItemValue( coredRemote, m_exceptionItemIds[ 0 ], 47, __LINE__ );
    setItemValue( coredRemote, m_triggerItemId, TriggerValue, __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "finished", __LINE__ );
    checkNoBroadcastReceived( "Action should be blocked by exception", __LINE__, std::chrono::seconds( 1 ) );
}

TEST_F( ActionExceptionsComponentTest, exceptionDoesntAffectOtherActions_ignoreResultPolicy ) {  // 3.1
    testExceptionDoesntAffectOtherActions( "ignore_result" );
}

TEST_F( ActionExceptionsComponentTest, exceptionDoesntAffectOtherActions_checkResultPolicy ) {  // 3.2
    testExceptionDoesntAffectOtherActions( "check_result" );
}

TEST_F( ActionExceptionsComponentTest, exceptionDoesntAffectOtherActions_waitResultPolicy ) {  // 3.3
    testExceptionDoesntAffectOtherActions( "wait_result" );
}

TEST_F( ActionExceptionsComponentTest, allActionsBlockedByExceptions ) {  // 4.
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    const SceneData sceneData = setupTestWithThreeActions( "", __LINE__ );
    const SceneID& sceneId = sceneData.first;
    const Json& sceneJson = sceneData.second;

    setItemValue( coredRemote, m_exceptionItemIds[ 0 ], 47, __LINE__ );
    setItemValue( coredRemote, m_exceptionItemIds[ 1 ], 47, __LINE__ );
    setItemValue( coredRemote, m_exceptionItemIds[ 2 ], 47, __LINE__ );
    setItemValue( coredRemote, m_triggerItemId, TriggerValue, __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "finished", __LINE__ );
    checkNoBroadcastReceived( "All actions should be blocked by exceptions", __LINE__, std::chrono::seconds( 1 ) );
}

TEST_F( ActionExceptionsComponentTest, exceptionStateCannotBeEvaluatedOneAction ) {  // 5.1
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    auto device1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto device2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto device3 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    std::string externalSceneId;
    std::string externalBlockId;
    {
        const Json whenBlock = isItemStateJson( device1.m_itemId, TriggerValue );
        const Json sceneJson1 = createSceneJson( { whenBlock }, {}, {} );
        Json result;
        externalSceneId = createScene( sceneJson1, result, __LINE__ );
        externalBlockId = result[ "when" ][ 0 ][ "_id" ].get< std::string >();
    }

    Json control = blockReferenceExceptionJson( externalSceneId, externalBlockId );
    const Json thenBlock = setItemValueJson( device2.m_itemId, 1, std::move( control ) );
    const Json whenBlock = isItemStateJson( device3.m_itemId, TriggerValue );
    const Json sceneJson = createSceneJson( { whenBlock }, { thenBlock }, {} );
    const SceneID sceneId = createScene( sceneJson, __LINE__ );

    deleteScene( externalSceneId, __LINE__ );

    setItemValue( coredRemote, device3.m_itemId, TriggerValue, __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "failed", __LINE__ );
}

TEST_F( ActionExceptionsComponentTest, exceptionStateCannotBeEvaluatedMultipleActions ) {  // 5.2
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    generateItems( 3, __LINE__ );

    std::string externalSceneId;
    std::string externalBlockId;
    {
        auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
        const Json whenBlock = isItemStateJson( device.m_itemId, TriggerValue );
        const Json sceneJson1 = createSceneJson( { whenBlock }, {}, {} );
        Json result;
        externalSceneId = createScene( sceneJson1, result, __LINE__ );
        externalBlockId = result[ "when" ][ 0 ][ "_id" ].get< std::string >();
    }

    Json control = blockReferenceExceptionJson( externalSceneId, externalBlockId );
    const Json thenBlock1 = setItemValueJson( m_exceptionItemIds[ 0 ], SetterValue1, std::move( control ) );
    const Json thenBlock2 = setItemValueJson( m_exceptionItemIds[ 1 ], SetterValue2, Json() );
    const Json thenBlock3 = setItemValueJson( m_exceptionItemIds[ 2 ], SetterValue3, Json() );
    const Json whenBlock = isItemStateJson( m_triggerItemId, TriggerValue );
    const Json sceneJson = createSceneJson( { whenBlock }, { thenBlock1, thenBlock2, thenBlock3 }, {} );
    const SceneID sceneId = createScene( sceneJson, __LINE__ );

    deleteScene( externalSceneId, __LINE__ );

    setItemValueWithoutCheck( coredRemote, m_triggerItemId, TriggerValue, __LINE__ );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "partially_finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_triggerItemId, TriggerValue, __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 1 ], SetterValue2, __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 2 ], SetterValue3, __LINE__ ) );
}

TEST_F( ActionExceptionsComponentTest, usingExpression ) {  // 6.
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    generateItems( 1, __LINE__ );

    const std::string expressionName = "expr";
    Json expressionCommand = replaceTokens(
        setExpressionCommand, { { "__NAME__", expressionName }, { "__CODE__", "" }, { "__VALUE_TYPE__", "bool" } } );
    expressionCommand[ "params" ][ "variable" ] = true;
    expressionCommand[ "params" ][ "value" ] = true;
    const JsonObject answer = nma().sendtoHubAndGetAnswer( expressionCommand );
    ASSERT_EQ( answer[ "error" ].dump(), Json().dump() );
    checkBroadcast( "", "hub.expression.added", { { "name", expressionName } }, __LINE__ );

    Json control = expressionReferenceExceptionJson( expressionName );
    const Json thenBlock = setItemValueJson( m_exceptionItemIds[ 0 ], SetterValue1, std::move( control ) );
    const Json whenBlock = isItemStateJson( m_triggerItemId, TriggerValue );
    const Json sceneJson = createSceneJson( { whenBlock }, { thenBlock }, {} );
    const SceneID sceneId = createScene( sceneJson, __LINE__ );

    setItemValueWithoutCheck( coredRemote, m_triggerItemId, TriggerValue, __LINE__ );

    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( sceneJson, sceneId, "finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_triggerItemId, TriggerValue, __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( m_exceptionItemIds[ 0 ], SetterValue1, __LINE__ ) );

    // cleanup
    HubPlatform::ScenesManager::RemoteInterface scenesRemote( nma() );
    scenesRemote.deleteSceneExpression( expressionName );
    checkBroadcast( "", "hub.expression.deleted", { { "name", expressionName } }, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
