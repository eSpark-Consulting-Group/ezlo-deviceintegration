#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/*

TEST PLAN

DONE    1. Function For/Less scene state control
DONE        1.1 For/Less operation ok, scenes state changed accordingly
DONE        1.2 For/Less operation failed, scenes state left unchanged
DONE    2. Function For/Least scene state control
DONE        2.1 For/Least operation ok, scenes state changed accordingly
DONE        2.2 For/Least operation failed, scenes state left unchanged
DONE    3. Function For/<any> for multiple children
DONE        3.1 For/Less operation for two children ok, scenes state changed accordingly
DONE        3.2 For/Least operation for two children ok, scenes state changed accordingly
DONE    4 Function Repeat
DONE        4.1 Function/Repeat with one child EventOperator
DONE        4.2 Function/Repeat with Nested Child Operators
DONE    5 Function Follow
DONE        5.1 Function/Follow with one child EventOperator
DONE        5.2 Function/Follow with two children
DONE        5.3 Function/Follow only one transition to false occurs
DONE    6 Function Pulse
DONE        6.1 Function/Pulse with one child EventOperator
DONE        6.2 Function/Pulse with two children
DONE    7 Function Latch
DONE        7.1 Function/Latch individual test
DONE        7.2 Function/Latch as a part of AND operator

*/

namespace HubPlatform {
namespace Tests {

class FunctionOperatorStateControlTests : public SceneManagerTestsFixture {
public:
    FunctionOperatorStateControlTests();
    void SetUp() override;
    void TearDown() override;

    void initSingleChildTest( const std::string& sceneNamePrefix,
                              std::string functionSettings,
                              bool addElseBlock,
                              int codeLine );
    void initMultiChildTest( const std::string& sceneNamePrefix, std::string functionSettings, int codeLine );
    void initNestedChildTest( const std::string& sceneNamePrefix, std::string functionSettings, int codeLine );

    void createSceneFinishedListener( const std::string& sceneNamePrefix, int codeLine );
    void createItemStateChangedListener( const std::string& sceneNamePrefix, int codeLine );

    static Json createSceneJsonWithSingleChild( const std::string& sceneNamePrefix,
                                                std::string functionSettings,
                                                std::string itemId );
    static Json createSceneJsonWithMultipleChildren( const std::string& sceneNamePrefix,
                                                     std::string functionSettings,
                                                     std::string itemId1,
                                                     std::string itemId2 );
    static std::string buildCompareNumbersGreater( std::string itemId,
                                                   int value = 50,
                                                   const char* type = "int",
                                                   const char* comparator = ">" );
    static Json buildSceneRequest( std::string name,
                                   std::string whenBlocks,
                                   std::string thenBlocks = "",
                                   std::string executionPolicy = "ignore_result" );

    static Json createOrOperatorWithMultipleChildren( const std::string& itemId );
    static Json createItemStateThenBlock( const std::string& itemId, int value );

    static constexpr int ElseBlockSetterValue = 40;

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
    CoreNmaTest::DeviceWithOneItemData m_deviceItem;
    CoreNmaTest::DeviceWithOneItemData m_deviceItem2;
    std::string m_sceneId1;
    Json m_functionSceneRequest;
    std::string m_sceneId2;
    Json m_listenerSceneRequest;
};

FunctionOperatorStateControlTests::FunctionOperatorStateControlTests()
: m_remote( makeRemote() ) {}

void FunctionOperatorStateControlTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void FunctionOperatorStateControlTests::TearDown() {
    clearExistingDevices( __LINE__ );
    SceneManagerTestsFixture::TearDown();
}

void FunctionOperatorStateControlTests::initSingleChildTest( const std::string& sceneNamePrefix,
                                                             std::string functionSettings,
                                                             bool addElseBlock,
                                                             int codeLine ) {
    m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );

    m_functionSceneRequest =
        createSceneJsonWithSingleChild( sceneNamePrefix, std::move( functionSettings ), m_deviceItem.m_itemId );

    if( addElseBlock ) {
        m_deviceItem2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );

        KeyStringList replaceList = { { "__ITEM_ID__", m_deviceItem2.m_itemId },
                                      { "__TYPE__", "int" },
                                      { "__VALUE__", std::to_string( ElseBlockSetterValue ) } };
        Json elseBlock = replaceTokens( setItemValueThenBlock, replaceList );
        m_functionSceneRequest[ "params" ][ "else" ] = Json::array( { std::move( elseBlock ) } );
    }

    m_sceneId1 = createScene( m_functionSceneRequest, codeLine );
}

void FunctionOperatorStateControlTests::initMultiChildTest( const std::string& sceneNamePrefix,
                                                            std::string functionSettings,
                                                            int codeLine ) {
    // device/item setup
    m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );
    m_deviceItem2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );

    // scene setup
    m_functionSceneRequest = createSceneJsonWithMultipleChildren(
        sceneNamePrefix, std::move( functionSettings ), m_deviceItem.m_itemId, m_deviceItem2.m_itemId );
    m_sceneId1 = createScene( m_functionSceneRequest, codeLine );

    createSceneFinishedListener( sceneNamePrefix, codeLine );
}

void FunctionOperatorStateControlTests::initNestedChildTest( const std::string& sceneNamePrefix,
                                                             std::string functionSettings,
                                                             int codeLine ) {
    // device/item setup
    m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );
    auto deviceId = m_deviceItem.m_deviceId;
    auto itemId = m_deviceItem.m_itemId;

    // scene setup: should modify scene state only from a function operator, not from any of its children, direct or
    // indirect root -> function ->  [ child1, not( compareNumbers, item1), or(compareNumbers, item1) ]

    Json whenBlock1 = Json::parse( buildCompareNumbersGreater( itemId, 50, "int", "<=" ) );
    Json whenBlock2 = Json::parse( buildCompareNumbersGreater( itemId ) );

    Json notBlock = replaceTokens( notOperatorWhenBlock, { { "__BLOCK__", whenBlock1.dump() } } );
    Json orBlock = replaceTokens( orOperatorWhenBlock, { { "__BLOCKS__", whenBlock2.dump() } } );

    auto functionOperatorStr = replaceTokensInString(
        functionOperatorWhenBlock,
        { { "__BLOCKS__", notBlock.dump() + ", " + orBlock.dump() }, { "__FUNCTION__", functionSettings } } );
    Json functionOperatorForLess1 = Json::parse( functionOperatorStr );

    m_functionSceneRequest = buildSceneRequest( sceneNamePrefix, functionOperatorForLess1.dump() );
    m_sceneId1 = createScene( m_functionSceneRequest, codeLine );

    createSceneFinishedListener( sceneNamePrefix, codeLine );
}

void FunctionOperatorStateControlTests::createSceneFinishedListener( const std::string& sceneNamePrefix,
                                                                     int codeLine ) {
    if( m_sceneId1.empty() ) {
        ADD_FAILURE() << "First scene was not created, cannot create listener if the main scene does not exist";
        return;
    }

    std::string isSceneState =
        replaceTokensInString( isSceneStateWhenBlock, { { "__SCENE_ID__", m_sceneId1 }, { "__STATE__", "finished" } } );

    m_listenerSceneRequest = replaceTokens( createSceneRequest,
                                            { { "__NAME__", sceneNamePrefix + "IsSceneEnd" },
                                              { "__WHEN_BLOCKS__", std::move( isSceneState ) },
                                              { "__THEN_BLOCKS__", "" } } );
    m_sceneId2 = createScene( m_listenerSceneRequest, codeLine );
}

void FunctionOperatorStateControlTests::createItemStateChangedListener( const std::string& sceneNamePrefix,
                                                                        int codeLine ) {
    if( m_deviceItem2.m_itemId.empty() ) {
        ADD_FAILURE() << "Second item was not created, cannot create listener if the item does not exist";
        return;
    }

    std::string isItemState = replaceTokensInString( isItemStateWhenBlock,
                                                     { { "__ITEM_ID__", m_deviceItem2.m_itemId },
                                                       { "__TYPE__", "int" },
                                                       { "__VALUE__", std::to_string( ElseBlockSetterValue ) } } );
    m_listenerSceneRequest = replaceTokens( createSceneRequest,
                                            { { "__NAME__", sceneNamePrefix + "Listener" },
                                              { "__WHEN_BLOCKS__", std::move( isItemState ) },
                                              { "__THEN_BLOCKS__", "" } } );
    m_sceneId2 = createScene( m_listenerSceneRequest, codeLine );
}

Json FunctionOperatorStateControlTests::createSceneJsonWithSingleChild( const std::string& sceneNamePrefix,
                                                                        std::string functionSettings,
                                                                        std::string itemId ) {
    std::string whenBlockStr = buildCompareNumbersGreater( itemId );
    std::string functionOperatorStr = replaceTokensInString(
        functionOperatorWhenBlock,
        { { "__BLOCKS__", std::move( whenBlockStr ) }, { "__FUNCTION__", std::move( functionSettings ) } } );

    return buildSceneRequest( "someFnScene", std::move( functionOperatorStr ) );
}

std::string FunctionOperatorStateControlTests::buildCompareNumbersGreater( std::string itemId,
                                                                           int value,
                                                                           const char* type,
                                                                           const char* comparator ) {
    if( !type || !*type )
        type = "int";
    if( !comparator || !*comparator )
        comparator = ">";
    return replaceTokensInString( compareNumbersWhenBlock,
                                  { { "__COMPARATOR__", comparator },
                                    { "__VALUE__", std::to_string( value ) },
                                    { "__TYPE__", type },
                                    { "__SOURCE_NAME__", "item" },
                                    { "__SOURCE_TYPE__", "item" },
                                    { "__SOURCE_VALUE__", "\"" + itemId + "\"" } } );
}

Json FunctionOperatorStateControlTests::buildSceneRequest( std::string name,
                                                           std::string whenBlocks,
                                                           std::string thenBlocks,
                                                           std::string executionPolicy ) {
    if( executionPolicy.empty() )
        executionPolicy = "ignore_result";
    return replaceTokens( createSceneRequest,
                          { { "__NAME__", std::move( name ) },
                            { "__EXEC_POLICY__", std::move( executionPolicy ) },
                            { "__WHEN_BLOCKS__", std::move( whenBlocks ) },
                            { "__THEN_BLOCKS__", std::move( thenBlocks ) } } );
}

Json FunctionOperatorStateControlTests::createSceneJsonWithMultipleChildren( const std::string& sceneNamePrefix,
                                                                             std::string functionSettings,
                                                                             std::string itemId1,
                                                                             std::string itemId2 ) {
    const std::string childWhenBlock1 = buildCompareNumbersGreater( std::move( itemId1 ) );
    const std::string childWhenBlock2 = buildCompareNumbersGreater( std::move( itemId2 ) );

    std::string functionOperator = replaceTokensInString( functionOperatorWhenBlock,
                                                          { { "__BLOCKS__", childWhenBlock1 + ", " + childWhenBlock2 },
                                                            { "__FUNCTION__", std::move( functionSettings ) } } );

    return buildSceneRequest( sceneNamePrefix, std::move( functionOperator ) );
}

Json FunctionOperatorStateControlTests::createOrOperatorWithMultipleChildren( const std::string& itemId ) {
    const std::string childWhenBlock1 = buildCompareNumbersGreater( itemId );
    const std::string childWhenBlock2 = buildCompareNumbersGreater( itemId );

    return replaceTokensInString( orOperatorWhenBlock, { { "__BLOCKS__", childWhenBlock1 + ", " + childWhenBlock2 } } );
}

Json FunctionOperatorStateControlTests::createItemStateThenBlock( const std::string& itemId, int value ) {
    return replaceTokens(
        setItemValueThenBlock,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "int" }, { "__VALUE__", std::to_string( value ) } } );
}

TEST_F( FunctionOperatorStateControlTests, functionForLessStateControl ) /* 1.1 */ {
    std::string functionSettings = R"("for": { "method": "less", "seconds": 1})";
    initSingleChildTest( "ForLess1", functionSettings, false, __LINE__ );
    createSceneFinishedListener( "ForLess1", __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );

    // expect for/less 1s is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionForLessStateFailed ) /* 1.2 */ {
    std::string functionSettings = R"("for": { "method": "less", "seconds": 1})";
    initSingleChildTest( "ForLess1", functionSettings, false, __LINE__ );
    createSceneFinishedListener( "ForLess1", __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1010 ) );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );

    // expect for/less 1s is false
    checkNoBroadcastReceived( "scene: " + m_sceneId1, __LINE__, std::chrono::seconds( 1 ) );
    ignoreBroadcasts(
        []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress"; },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );
}

TEST_F( FunctionOperatorStateControlTests, functionForLeastStateControl ) /* 2.1 */ {
    std::string functionSettings = R"("for": { "method": "least", "seconds": 1})";
    initSingleChildTest( "ForLeast1", functionSettings, false, __LINE__ );
    createSceneFinishedListener( "ForLeast1", __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1010 ) );

    // expect for/less 1s is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionForLeastStateFailed ) /* 2.2 */ {
    std::string functionSettings = R"("for": { "method": "least", "seconds": 1})";
    initSingleChildTest( "ForLeast1", functionSettings, false, __LINE__ );
    createSceneFinishedListener( "ForLeast1", __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );

    // expect for/less 1s is false
    checkNoBroadcastReceived( "scene: " + m_sceneId1, __LINE__, std::chrono::seconds( 1 ) );
}

TEST_F( FunctionOperatorStateControlTests, forLessForMultiChildren ) /* 3.1 */ {
    std::string functionSettings = R"("for": { "method": "less", "seconds": 1})";
    initMultiChildTest( "ForLessMulti", functionSettings, __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem2.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );

    // expect for/less 1s is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, forLeastForMultiChildren ) /* 3.2 */ {
    std::string functionSettings = R"("for": { "method": "least", "seconds": 1})";
    initMultiChildTest( "ForLeastMulti", functionSettings, __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem2.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1010 ) );

    // expect for/least is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionForLessWithNestedChildOperators ) /* 3.3 */ {
    std::string functionSettings = R"("for": { "method": "less", "seconds": 1})";
    initNestedChildTest( "for", functionSettings, __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );

    // expect repeat is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionRepeatStateControl ) /* 4.1 */ {
    std::string functionSettings = R"("repeat": { "times": 2, "seconds": 1})";
    initSingleChildTest( "Repeat1", functionSettings, false, __LINE__ );
    createSceneFinishedListener( "Repeat1", __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );

    // expect repeat is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionRepeatWithNestedChildOperators ) /* 4.2 */ {
    std::string functionSettings = R"("repeat": { "times": 2, "seconds": 1})";
    initNestedChildTest( "repeat", functionSettings, __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );

    // expect repeat is true
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionFollowStateControl ) /* 5.1 */ {
    std::string functionSettings = R"("follow": { "delayReset": 2 })";
    initSingleChildTest( "follow1", std::move( functionSettings ), true, __LINE__ );
    createItemStateChangedListener( "follow1", __LINE__ );

    // trigger a scene, switch to "true" happens instantly
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // reset scene to "false", this operation is delayed
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 1900 ) );
    checkItemUpdateBroadcast( m_deviceItem2.m_itemId, ElseBlockSetterValue, __LINE__ );  // from else block
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionFollowWithMultipleChildren ) /* 5.2 */ {
    // setup
    const std::string sceneNamePrefix = "follow2";

    m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    m_deviceItem2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    m_functionSceneRequest = createSceneJsonWithMultipleChildren(
        sceneNamePrefix, R"("follow": { "delayReset": 2 })", m_deviceItem.m_itemId, m_deviceItem2.m_itemId );
    m_functionSceneRequest[ "params" ][ "else" ] =
        Json::array( { createItemStateThenBlock( m_deviceItem2.m_itemId, ElseBlockSetterValue ) } );
    m_sceneId1 = createScene( m_functionSceneRequest, __LINE__ );

    createItemStateChangedListener( sceneNamePrefix, __LINE__ );

    // trigger a scene, switch to "true" happens instantly
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem2.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // reset scene to "false", this operation is delayed
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 1900 ) );
    checkItemUpdateBroadcast( m_deviceItem2.m_itemId, ElseBlockSetterValue, __LINE__ );  // from else block
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionFollowElseBlockTriggeredOnce ) /* 5.3 */ {
    // setup
    const std::string sceneNamePrefix = "follow2";

    m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    m_deviceItem2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    std::string orOperator = createOrOperatorWithMultipleChildren( m_deviceItem.m_itemId );
    std::string functionOperator = replaceTokensInString(
        functionOperatorWhenBlock,
        { { "__BLOCKS__", std::move( orOperator ) }, { "__FUNCTION__", R"("follow": { "delayReset": 2 })" } } );

    m_functionSceneRequest = buildSceneRequest( sceneNamePrefix, functionOperator );

    // 2 else blocks are needed, because if the item's value is changed to same value - the broadcast is not sent
    Json elseBlock1 = createItemStateThenBlock( m_deviceItem2.m_itemId, 30 );
    Json elseBlock2 = createItemStateThenBlock( m_deviceItem2.m_itemId, 40 );
    m_functionSceneRequest[ "params" ][ "else" ] = Json::array( { std::move( elseBlock1 ), std::move( elseBlock2 ) } );
    m_sceneId1 = createScene( m_functionSceneRequest, __LINE__ );

    createItemStateChangedListener( sceneNamePrefix, __LINE__ );

    // trigger a scene, switch to "true" happens instantly
    setItemValue( m_remote, m_deviceItem2.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // reset scene to "false", this operation is delayed
    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 1900 ) );
    checkItemUpdateBroadcast( m_deviceItem2.m_itemId, 30, __LINE__ );  // from else block
    checkItemUpdateBroadcast( m_deviceItem2.m_itemId, 40, __LINE__ );  // from else block
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 300 ) );
}

TEST_F( FunctionOperatorStateControlTests, functionPulseStateControl ) /* 6.1 */ {
    std::string functionSettings = R"("pulse": { "truePeriod": 1, "times": 1 })";
    initSingleChildTest( "pulse1", std::move( functionSettings ), true, __LINE__ );
    createItemStateChangedListener( "pulse1", __LINE__ );

    // trigger a scene, switch to "true" happens instantly
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // reset scene to "false", this operation is delayed
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 900 ) );
    checkItemUpdateBroadcast( m_deviceItem2.m_itemId, ElseBlockSetterValue, __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, functionPulseWithMultipleChildren ) /* 6.2 */ {
    // setup
    const std::string sceneNamePrefix = "pulse2";

    m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    m_deviceItem2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    m_functionSceneRequest = createSceneJsonWithMultipleChildren(
        sceneNamePrefix, R"("pulse": { "truePeriod": 1, "times": 1 })", m_deviceItem.m_itemId, m_deviceItem2.m_itemId );
    m_functionSceneRequest[ "params" ][ "else" ] =
        Json::array( { createItemStateThenBlock( m_deviceItem2.m_itemId, ElseBlockSetterValue ) } );
    m_sceneId1 = createScene( m_functionSceneRequest, __LINE__ );

    createItemStateChangedListener( sceneNamePrefix, __LINE__ );

    // trigger a scene, switch to "true" happens instantly
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    setItemValue( m_remote, m_deviceItem2.m_itemId, 70, __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // reset scene to "false", this operation is delayed
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 900 ) );
    checkItemUpdateBroadcast( m_deviceItem2.m_itemId, ElseBlockSetterValue, __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );
}

TEST_F( FunctionOperatorStateControlTests, latchStateControl ) /* 7.1 */ {
    std::string functionSettings = R"("latch": { "enabled": true})";
    initSingleChildTest( "Latch/7.1", functionSettings, false, __LINE__ );
    createSceneFinishedListener( "Latch/7.1", __LINE__ );

    // trigger a scene
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    // should run here
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

    // expect isSceneState is ok, scene status = 'finished'
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );

    setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 200 ) );

    // no run
    setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
    // no run, latched
    checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 200 ) );
}

TEST_F( FunctionOperatorStateControlTests, latchInAndOperator ) /* 7.2 */ {
    std::string functionSettings = R"("latch": { "enabled": true })";
    // set up
    {
        m_deviceItem = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
        m_deviceItem2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

        // scene setup

        // latch block
        std::string latchChildWhenBlock = buildCompareNumbersGreater( m_deviceItem.m_itemId );
        std::string functionOperatorStr =
            replaceTokensInString( functionOperatorWhenBlock,
                                   { { "__BLOCKS__", latchChildWhenBlock }, { "__FUNCTION__", functionSettings } } );

        // individual event
        std::string individualWhenBlock = buildCompareNumbersGreater( m_deviceItem2.m_itemId );

        std::string whenBlocks = functionOperatorStr + ", " + individualWhenBlock;
        std::string unlatchingAndOperator =
            replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", whenBlocks } } );

        m_functionSceneRequest = buildSceneRequest( "Latch/7.2", unlatchingAndOperator );
        m_sceneId1 = createScene( m_functionSceneRequest, __LINE__ );

        createSceneFinishedListener( "Latch/7.2", __LINE__ );
    }

    for( int loop = 0; loop < 2; loop++ ) {
        // trigger a scene
        setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );   // latches
        setItemValue( m_remote, m_deviceItem2.m_itemId, 70, __LINE__ );  // makes AND operator True
        // scene should run here
        testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "started", __LINE__ );
        testRunProgressBroadcast( m_functionSceneRequest, m_sceneId1, "finished", __LINE__ );

        // expect isSceneState is ok, scene status = 'finished'
        testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "started", __LINE__ );
        testRunProgressBroadcast( m_listenerSceneRequest, m_sceneId2, "finished", __LINE__ );

        // latch should stay closed, setting latch(child) to false should not change AND operator state, True
        setItemValue( m_remote, m_deviceItem.m_itemId, 10, __LINE__ );
        checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 200 ) );

        // no run, AND already True
        setItemValue( m_remote, m_deviceItem.m_itemId, 70, __LINE__ );
        checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 200 ) );

        // makes AND operator False, unlatches the latch function
        setItemValue( m_remote, m_deviceItem2.m_itemId, 10, __LINE__ );
        checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 200 ) );

        // prepare next loop, make event1 False
        setItemValue( m_remote, m_deviceItem.m_itemId, 11, __LINE__ );
        checkNoBroadcastReceived( "scene: " + m_sceneId2, __LINE__, std::chrono::milliseconds( 200 ) );

        // start second run, should latch an item successfully again
    }
}

}  // namespace Tests
}  // namespace HubPlatform
