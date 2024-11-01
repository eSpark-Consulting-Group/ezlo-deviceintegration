#include "scenes_manager/scenes_processor/parsers/action_parser.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "cored/remote_interface/cored_interface_factory.h"

#include <gtest/gtest.h>

/**

TESTPLAN

DONE    1. Action parsed successfully (simple case)
DONE    2. "setItemValue" action with temperature
DONE    3. Parsing errors when mandatory parameters are absent
DONE    4.1 Unite actions
DONE    4.2 multicastOrderWithIgnoreExecutionPolicy
DONE    4.3 multicastOrderWithCheckExecutionPolicy
DONE    4.4 multicastOrderWithIgnoreAndWaitExecutionPolicy
DONE    5. "runPluginScript" action check
DONE    6. Action exceptions
DONE        6.1 no exceptions
DONE        6.2 reference to a block in the same scene
DONE        6.3 reference to a block in other scene
DONE        6.4 reference to an exception
DONE    7. "runHubBackup" action
DONE        7.1 successfull parse
DONE        7.2 exception throw
DONE    8. "forwardNotificatiom" action
DONE        8.1 successfull parse

 */

namespace SceneEngine {

static bool operator==( const ActionExceptionData::BlockReference& left,
                        const ActionExceptionData::BlockReference& right ) {
    return left.sceneId == right.sceneId && left.blockId == right.blockId;
}

static bool operator==( const ActionExceptionData::ExpressionReference& left,
                        const ActionExceptionData::ExpressionReference& right ) {
    return left.name == right.name;
}

}  // namespace SceneEngine

namespace HubPlatform {
namespace ScenesManager {
namespace UT {

// A tuple of test params: (block_name_str, block_type_enum)
// block_name_str: the name of a root element in block json (either "then" or "else" for now)
// block_type_enum: the type of action block that corresponds to block_name_str, used in calls to parse() method
using ActionParserTestParam = std::tuple< std::string, ScenesManager::ActionParser::BlockType >;

class ActionParserTest : public testing::Test, public testing::WithParamInterface< ActionParserTestParam > {
public:
    void SetUp() override;
    SceneEngine::ActionBlocks::Actions parse( Json& sceneJsonObject, ScenesManager::ActionParser::BlockType blockType );
    static JsonObject createSetItemValueThenBlock( const std::string& itemId,
                                                   const std::string& valueType,
                                                   const std::string& valueValue );
    static JsonObject createRunPluginScript( const std::string& scriptId, const Json& params );
    static JsonObject createRunPluginScriptWithExecution( const std::string& scriptId, const Json& params );

protected:
    // Test params:
    std::string m_blockName;
    ScenesManager::ActionParser::BlockType m_blockType;

private:
    Services::UT::FakeService m_service;
    CoreDRemoteInterface::CoreDInterfaceUPtr m_core;
    std::unique_ptr< ExtensionsManager::RemoteInterface > m_extensionManager;
    std::unique_ptr< ScenesManager::ActionParser > m_actionParser;
    IdUtils::PlaceholdersMapping m_placeholdersMapping;
};

void ActionParserTest::SetUp() {
    m_core = CoreDRemoteInterface::makeInstance( m_service );
    m_extensionManager = std::make_unique< ExtensionsManager::RemoteInterface >( m_service );
    m_actionParser = std::make_unique< ScenesManager::ActionParser >( "", *m_core, *m_extensionManager, nullptr );
    m_blockName = std::get< 0 >( GetParam() );
    m_blockType = std::get< 1 >( GetParam() );
}

SceneEngine::ActionBlocks::Actions ActionParserTest::parse( Json& sceneJsonObject,
                                                            ScenesManager::ActionParser::BlockType blockType ) {
    return m_actionParser->parse( sceneJsonObject, blockType, m_placeholdersMapping );
}

JsonObject ActionParserTest::createSetItemValueThenBlock( const std::string& itemId,
                                                          const std::string& valueType,
                                                          const std::string& valueValue ) {
    JsonObject block;
    block[ "blockOptions" ] = R"(
        {
            "method":{
                "name":"setItemValue",
                "args":{
                    "item":"item",
                    "value":"value"
                }
            }
        }
        )"_json;

    block[ "blockType" ] = "then";

    JsonObject f1 = { { "name", "item" }, { "type", "item" }, { "value", itemId } };
    JsonObject f2 = { { "name", "value" }, { "type", valueType }, { "value", valueValue } };

    block[ "fields" ] = JSON_ARRAY_( f1, f2 );
    return block;
}

JsonObject ActionParserTest::createRunPluginScript( const std::string& scriptId, const Json& params ) {
    JsonObject block;
    block[ "blockOptions" ] = R"(
        {
            "method":{
                "name": "runPluginScript",
                "args": {
                    "script": "script",
                    "params": "params"
                }
            }
        })"_json;

    block[ "blockType" ] = "then";

    JsonObject f1 = { { "name", "script" }, { "type", "string" }, { "value", scriptId } };
    JsonObject f2 = { { "name", "params" }, { "type", "object" }, { "value", params } };

    block[ "fields" ] = JSON_ARRAY_( f1, f2 );
    return block;
}

JsonObject ActionParserTest::createRunPluginScriptWithExecution( const std::string& scriptId, const Json& params ) {
    JsonObject block;
    block[ "blockOptions" ] = R"(
    {

        "method":{
            "name": "runPluginScript",
            "args": {
                "script": "script",
                "params": "params"
            }
        }
    })"_json;

    block[ "blockType" ] = "then";

    JsonObject f1 = { { "name", "script" }, { "type", "string" }, { "value", scriptId } };
    JsonObject f2 = { { "name", "params" }, { "type", "object" }, { "value", params } };

    block[ "fields" ] = JSON_ARRAY_( f1, f2 );
    block[ "exec_policy" ] = "wait_result";
    return block;
}

TEST_P( ActionParserTest, checkActions ) {  // 1.
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = SceneEngine::UT::thenBlockArray;
    SceneEngine::ActionBlocks::Actions actions = parse( sceneJson, m_blockType );
    ASSERT_EQ( actions.size(), SceneEngine::UT::thenBlockArray.size() );
    EXPECT_EQ( actions[ 0 ]->getName(), "setItemValue" );
    EXPECT_EQ( actions[ 0 ]->getArgs().dump(), std::string( R"({"item":"897607_32771_1","value":10})" ) );
    EXPECT_EQ( actions[ 0 ]->getDelay(), std::chrono::seconds( 5412 ) );
    EXPECT_EQ( actions[ 1 ]->getName(), "setDeviceArmed" );
    EXPECT_EQ( actions[ 1 ]->getArgs().dump(), std::string( R"({"device":"76asdasd76yasf89","deviceFlag":false})" ) );
    EXPECT_EQ( actions[ 1 ]->getDelay(), std::chrono::seconds( 0 ) );
}

// TODO check setItemValue action for temperature item, where arguments.value contains a 'scale' field
TEST_P( ActionParserTest, checkTemperatureSetItemValueAction ) {  // 2.
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = { SceneEngine::UT::setItemValueActionForTemperature };

    SceneEngine::ActionBlocks::Actions actions = parse( sceneJson, m_blockType );

    ASSERT_EQ( actions.size(), sceneJson[ m_blockName ].size() );
    const auto& action = *actions[ 0 ];
    EXPECT_EQ( action.getName(), "setItemValue" );
    std::string expectedArgs = R"({"item":"897607_32771_2","scale":"fahrenheit","value":78.7})";
    try {
        Json jExpectedArgs = Json::parse( expectedArgs );

        const Json& args = action.getArgs();
        EXPECT_EQ( args.size(), jExpectedArgs.size() ) << "number of fields in args should match";
        for( auto jit = args.begin(); jit != args.end(); jit++ ) {
            Json expectedValue;
            EXPECT_TRUE( json_get( args, jit.key(), expectedValue ) ) << "Field should exist: " << jit.key();
            if( jit.key() == "value" && jit.value().is_number() )
                EXPECT_FLOAT_EQ( expectedValue, jit.value() );
            else
                EXPECT_EQ( expectedValue, jit.value() );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + e.what() );
        FAIL() << e.what();
    }
}

TEST_P( ActionParserTest, errorsForAbsentParameters ) {  // 3.
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = SceneEngine::UT::thenBlockEmptyBlockOptions;
    EXPECT_THROW( parse( sceneJson, m_blockType ), SceneEngine::SceneException );
    sceneJson[ m_blockName ] = SceneEngine::UT::thenBlockEmptyMethod;
    EXPECT_THROW( parse( sceneJson, m_blockType ), SceneEngine::SceneException );
    sceneJson[ m_blockName ] = SceneEngine::UT::thenBlockEmptyName;
    EXPECT_THROW( parse( sceneJson, m_blockType ), SceneEngine::SceneException );
}

TEST_P( ActionParserTest, uniteActions ) {  // 4.1
    const JsonObject createSetItemValueMulticastAction = R"(
    {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetItemValue",
        "then" : [],
        "when" : []
    }
    )"_json;

    JsonObject block1 = createSetItemValueThenBlock( "id1", "int", "10" );
    JsonObject block2 = createSetItemValueThenBlock( "id2", "int", "10" );
    JsonObject block3 = createSetItemValueThenBlock( "id3", "float", "11.1" );
    JsonObject block4 = createSetItemValueThenBlock( "id4", "float", "11.1" );
    JsonObject block5 = createSetItemValueThenBlock( "id5", "float", "555" );
    JsonObject block6 = createSetItemValueThenBlock( "id6", "float", "11.1" );

    JsonObject creatSceneJson = createSetItemValueMulticastAction;
    JsonArray& thenBlocksArray = creatSceneJson[ m_blockName ];
    thenBlocksArray.push_back( std::move( block1 ) );
    thenBlocksArray.push_back( std::move( block2 ) );
    thenBlocksArray.push_back( std::move( block3 ) );
    thenBlocksArray.push_back( std::move( block4 ) );
    thenBlocksArray.push_back( std::move( block5 ) );
    thenBlocksArray.push_back( std::move( block6 ) );

    SceneEngine::ActionBlocks::Actions actions = parse( creatSceneJson, m_blockType );

    // Check that 2 similar action groups were united and one is left as is.
    constexpr size_t expectedActionsCount = 3;
    EXPECT_EQ( actions.size(), expectedActionsCount );
    for( size_t i = 0; i < expectedActionsCount; ++i ) {
        const JsonObject& args = actions[ i ]->getArgs();
        if( !args[ "item" ].is_array() ) {
            EXPECT_EQ( args[ "item" ].get< std::string >(), "id5" );
        }
        else {
            const std::set< std::string > set1{ "id1", "id2" };
            const std::set< std::string > set2{ "id3", "id4", "id6" };
            const std::set< std::string > s = args[ "item" ];
            EXPECT_TRUE( set1 == s || set2 == s );
        }
    }
}

TEST_P( ActionParserTest, multicastOrderWithIgnoreExecutionPolicy ) {  // 4.2
    const JsonObject createSetItemValueMulticastAction = R"(
    {
        "exec_policy": "ignore_result",
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetItemValue",
        "then" : [],
        "when" : []
    }
    )"_json;

    JsonObject block1 = createSetItemValueThenBlock( "id1", "int", "10" );
    JsonObject block2 = createSetItemValueThenBlock( "id2", "int", "10" );
    JsonObject block3 = createSetItemValueThenBlock( "id3", "int", "10" );
    JsonObject block4 = createSetItemValueThenBlock( "id4", "int", "10" );
    const Json scriptParams = { { "paramName", "paramValue" } };
    Json breakingAction = createRunPluginScript( "HUB:some_plugin/scripts/script_path", scriptParams );

    JsonObject creatSceneJson = createSetItemValueMulticastAction;
    JsonArray& thenBlocksArray = creatSceneJson[ m_blockName ];
    thenBlocksArray.push_back( std::move( block1 ) );
    thenBlocksArray.push_back( std::move( block2 ) );
    thenBlocksArray.push_back( std::move( breakingAction ) );
    thenBlocksArray.push_back( std::move( block3 ) );
    thenBlocksArray.push_back( std::move( block4 ) );

    SceneEngine::ActionBlocks::Actions actions = parse( creatSceneJson, m_blockType );

    constexpr size_t expectedActionsCount = 2;
    constexpr size_t expectedSetItemValueCount = 1;
    size_t setItemValueCount = 0;
    EXPECT_EQ( actions.size(), expectedActionsCount );
    for( size_t i = 0; i < expectedActionsCount; ++i ) {
        const JsonObject& args = actions[ i ]->getArgs();
        if( actions[ i ]->getName() == "setItemValue" ) {
            setItemValueCount++;
            const std::set< std::string > set1{ "id1", "id2", "id3", "id4" };
            const std::set< std::string > s = args[ "item" ];
            EXPECT_TRUE( set1 == s );
        }
    }
    EXPECT_EQ( expectedSetItemValueCount, setItemValueCount );
    EXPECT_EQ( actions[ 0 ]->getName(), "setItemValue" );
    EXPECT_EQ( actions[ 1 ]->getName(), "runPluginScript" );
}

TEST_P( ActionParserTest, multicastOrderWithCheckExecutionPolicy ) {  // 4.3
    const JsonObject createSetItemValueMulticastAction = R"(
    {
        "exec_policy": "check_result",
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetItemValue",
        "then" : [],
        "when" : []
    }
    )"_json;

    JsonObject block1 = createSetItemValueThenBlock( "id1", "int", "10" );
    JsonObject block2 = createSetItemValueThenBlock( "id2", "int", "10" );
    JsonObject block3 = createSetItemValueThenBlock( "id3", "int", "10" );
    JsonObject block4 = createSetItemValueThenBlock( "id4", "int", "10" );
    const Json scriptParams = { { "paramName", "paramValue" } };
    Json breakingAction = createRunPluginScript( "HUB:some_plugin/scripts/script_path", scriptParams );

    JsonObject creatSceneJson = createSetItemValueMulticastAction;
    JsonArray& thenBlocksArray = creatSceneJson[ m_blockName ];
    thenBlocksArray.push_back( std::move( block1 ) );
    thenBlocksArray.push_back( std::move( block2 ) );
    thenBlocksArray.push_back( std::move( breakingAction ) );
    thenBlocksArray.push_back( std::move( block3 ) );
    thenBlocksArray.push_back( std::move( block4 ) );

    SceneEngine::ActionBlocks::Actions actions = parse( creatSceneJson, m_blockType );

    constexpr size_t expectedActionsCount = 3;
    constexpr size_t expectedSetItemValueCount = 2;
    size_t setItemValueCount = 0;
    EXPECT_EQ( actions.size(), expectedActionsCount );
    for( size_t i = 0; i < expectedActionsCount; ++i ) {
        const JsonObject& args = actions[ i ]->getArgs();
        if( actions[ i ]->getName() == "setItemValue" ) {
            setItemValueCount++;
            const std::set< std::string > set1{ "id1", "id2" };
            const std::set< std::string > set2{ "id3", "id4" };
            const std::set< std::string > s = args[ "item" ];
            EXPECT_TRUE( set1 == s || set2 == s );
        }
    }
    EXPECT_EQ( expectedSetItemValueCount, setItemValueCount );
    EXPECT_EQ( actions[ 0 ]->getName(), "setItemValue" );
    EXPECT_EQ( actions[ 1 ]->getName(), "runPluginScript" );
    EXPECT_EQ( actions[ 2 ]->getName(), "setItemValue" );
}

TEST_P( ActionParserTest, multicastOrderWithIgnoreAndWaitExecutionPolicy ) {  // 4.4
    const JsonObject createSetItemValueMulticastAction = R"(
    {
        "exec_policy": "ignore_result",
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetItemValue",
        "then" : [],
        "when" : []
    }
    )"_json;

    JsonObject block1 = createSetItemValueThenBlock( "id1", "int", "10" );
    JsonObject block2 = createSetItemValueThenBlock( "id2", "int", "10" );
    JsonObject block3 = createSetItemValueThenBlock( "id3", "int", "10" );
    JsonObject block4 = createSetItemValueThenBlock( "id4", "int", "10" );
    const Json scriptParams = { { "paramName", "paramValue" } };
    Json breakingAction = createRunPluginScriptWithExecution( "HUB:some_plugin/scripts/script_path", scriptParams );

    JsonObject creatSceneJson = createSetItemValueMulticastAction;
    JsonArray& thenBlocksArray = creatSceneJson[ m_blockName ];
    thenBlocksArray.push_back( std::move( block1 ) );
    thenBlocksArray.push_back( std::move( block2 ) );
    thenBlocksArray.push_back( std::move( breakingAction ) );
    thenBlocksArray.push_back( std::move( block3 ) );
    thenBlocksArray.push_back( std::move( block4 ) );

    SceneEngine::ActionBlocks::Actions actions = parse( creatSceneJson, m_blockType );

    constexpr size_t expectedActionsCount = 3;
    constexpr size_t expectedSetItemValueCount = 2;
    size_t setItemValueCount = 0;
    EXPECT_EQ( actions.size(), expectedActionsCount );
    for( size_t i = 0; i < expectedActionsCount; ++i ) {
        const JsonObject& args = actions[ i ]->getArgs();
        if( actions[ i ]->getName() == "setItemValue" ) {
            setItemValueCount++;
            const std::set< std::string > set1{ "id1", "id2" };
            const std::set< std::string > set2{ "id3", "id4" };
            const std::set< std::string > s = args[ "item" ];
            EXPECT_TRUE( set1 == s || set2 == s );
        }
    }
    EXPECT_EQ( expectedSetItemValueCount, setItemValueCount );
    EXPECT_EQ( actions[ 0 ]->getName(), "setItemValue" );
    EXPECT_EQ( actions[ 1 ]->getName(), "runPluginScript" );
    EXPECT_EQ( actions[ 2 ]->getName(), "setItemValue" );
}

TEST_P( ActionParserTest, runPluginScriptCheck ) {  // 5.
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = SceneEngine::UT::thenBlockArray;

    const Json scriptParams = { { "paramName", "paramValue" } };
    Json thenBlock = createRunPluginScript( "HUB:some_plugin/scripts/script_path", scriptParams );
    sceneJson[ m_blockName ] = JSON_ARRAY_( thenBlock );

    SceneEngine::ActionBlocks::Actions actions = parse( sceneJson, m_blockType );
    ASSERT_EQ( actions.size(), sceneJson[ m_blockName ].size() );
    EXPECT_EQ( actions[ 0 ]->getName(), "runPluginScript" );
    EXPECT_TRUE( actions[ 0 ]->getArgs().count( "params" ) > 0 );
    EXPECT_EQ( actions[ 0 ]->getArgs()[ "params" ], scriptParams );
    EXPECT_EQ( actions[ 0 ]->getDelay(), std::chrono::seconds( 0 ) );
}

TEST_P( ActionParserTest, actionWithoutExceptions ) {  // 6.1
    using namespace SceneEngine;

    Json sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = JSON_ARRAY_( SceneEngine::UT::setItemValueAction );

    const ActionBlocks::Actions actions = parse( sceneJson, m_blockType );
    ASSERT_EQ( actions.size(), 1 );

    ActionExceptionData expected;
    const ActionExceptionData& actual = actions.front()->getExceptionData();
    EXPECT_EQ( expected.storage, actual.storage );
}

TEST_P( ActionParserTest, blockReferenceException ) {  // 6.2
    using namespace SceneEngine;

    const std::string sceneId = "scene_id";
    const std::string blockId = "block_id";

    Json sceneJson = SceneEngine::UT::emptyScene;
    Json action = SceneEngine::UT::setItemValueAction;
    action[ "control" ] =
        Tests::replaceTokens( SceneEngine::UT::blockReferenceExceptionTemplate, { { "__BLOCK_ID__", blockId } } );
    sceneJson[ m_blockName ] = JSON_ARRAY_( std::move( action ) );
    sceneJson[ "_id" ] = sceneId;

    const ActionBlocks::Actions actions = parse( sceneJson, m_blockType );
    ASSERT_EQ( actions.size(), 1 );

    const ActionExceptionData::BlockReference expected{ sceneId, blockId };
    const ActionExceptionData& actual = actions.front()->getExceptionData();
    EXPECT_EQ( ActionExceptionData{ expected }.storage, actual.storage );
}

TEST_P( ActionParserTest, externalBlockReferenceException ) {  // 6.3
    using namespace SceneEngine;

    const std::string sceneId = "scene_id";
    const std::string blockId = "block_id";

    Json sceneJson = SceneEngine::UT::emptyScene;
    Json action = SceneEngine::UT::setItemValueAction;
    action[ "control" ] = Tests::replaceTokens( SceneEngine::UT::externalBlockReferenceExceptionTemplate,
                                                { { "__BLOCK_ID__", blockId }, { "__SCENE_ID__", sceneId } } );
    sceneJson[ m_blockName ] = JSON_ARRAY_( std::move( action ) );

    const ActionBlocks::Actions actions = parse( sceneJson, m_blockType );
    ASSERT_EQ( actions.size(), 1 );

    const ActionExceptionData::BlockReference expected{ sceneId, blockId };
    const ActionExceptionData& actual = actions.front()->getExceptionData();
    EXPECT_EQ( ActionExceptionData{ expected }.storage, actual.storage );
}

TEST_P( ActionParserTest, expressionReferenceException ) {  // 6.4
    using namespace SceneEngine;

    const std::string name = "variable";

    Json sceneJson = SceneEngine::UT::emptyScene;
    Json action = SceneEngine::UT::setItemValueAction;
    action[ "control" ] =
        Tests::replaceTokens( SceneEngine::UT::expressionReferenceExceptionTemplate, { { "__NAME__", name } } );
    sceneJson[ m_blockName ] = JSON_ARRAY_( std::move( action ) );

    const ActionBlocks::Actions actions = parse( sceneJson, m_blockType );
    ASSERT_EQ( actions.size(), 1 );

    const ActionExceptionData::ExpressionReference expected{ name };
    const ActionExceptionData& actual = actions.front()->getExceptionData();
    EXPECT_EQ( ActionExceptionData{ expected }.storage, actual.storage );
}

TEST_P( ActionParserTest, runHubBackupCheck ) {  // 7.1
    using namespace SceneEngine;
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = JSON_ARRAY_( SceneEngine::UT::hubBackupRunAction );

    const ActionBlocks::Actions actions = parse( sceneJson, m_blockType );

    ASSERT_EQ( actions.size(), sceneJson[ m_blockName ].size() );
    EXPECT_EQ( actions[ 0 ]->getName(), "runHubBackup" );

    auto arguments = actions[ 0 ]->getArgs();
    ASSERT_TRUE( json_contains( arguments, "action" ) );
    ASSERT_TRUE( json_contains( arguments, "backupId" ) );
    jsonCheckString( arguments, "action" );
    jsonCheckObject( arguments, "backupId" );
}

TEST_P( ActionParserTest, runHubBackupCheckExeption ) {  // 7.2
    using namespace SceneEngine;
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto block = SceneEngine::UT::hubBackupRunAction;
    block[ "fields" ][ 1 ].erase( "value" );
    sceneJson[ m_blockName ] = JSON_ARRAY_( block );

    EXPECT_THROW( { const ActionBlocks::Actions actions = parse( sceneJson, m_blockType ); },
                  SceneEngine::SceneException );

    block = SceneEngine::UT::hubBackupRunAction;
    block[ "fields" ][ 1 ][ "value" ] = "key";
    sceneJson[ m_blockName ] = JSON_ARRAY_( block );

    EXPECT_THROW( { const ActionBlocks::Actions actions = parse( sceneJson, m_blockType ); },
                  SceneEngine::SceneException );
}

TEST_P( ActionParserTest, forwrdNonificationCheck ) {  // 8.1
    using namespace SceneEngine;
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ m_blockName ] = JSON_ARRAY_( SceneEngine::UT::hubForwardNotificationAction );

    const ActionBlocks::Actions actions = parse( sceneJson, m_blockType );

    ASSERT_EQ( actions.size(), sceneJson[ m_blockName ].size() );
    EXPECT_EQ( actions[ 0 ]->getName(), "forwardNotifications" );
    auto arguments = actions[ 0 ]->getArgs();

    ASSERT_TRUE( json_contains( arguments, "notificationTemplate" ) );
    jsonCheckString( arguments, "notificationTemplate" );
    ASSERT_TRUE( json_contains( arguments, "targets" ) );
    jsonCheckArray( arguments, "targets" );
    ASSERT_TRUE( json_contains( arguments, "device" ) );
    jsonCheckString( arguments, "device" );
    ASSERT_TRUE( json_contains( arguments, "item" ) );
    jsonCheckString( arguments, "item" );
    ASSERT_TRUE( json_contains( arguments, "notificationBlock" ) );
    jsonCheckObject( arguments, "notificationBlock" );
}

TEST_P( ActionParserTest, forwrdNonificationExeption ) {  // 8.2
    using namespace SceneEngine;
    for( int i = 0; i < 5; i++ ) {
        JsonObject sceneJson = SceneEngine::UT::emptyScene;
        auto block = SceneEngine::UT::hubForwardNotificationAction;
        block[ "fields" ][ i ].erase( "value" );
        sceneJson[ m_blockName ] = JSON_ARRAY_( block );
        EXPECT_THROW( { const ActionBlocks::Actions actions = parse( sceneJson, m_blockType ); },
                      SceneEngine::SceneException );
        block = SceneEngine::UT::hubForwardNotificationAction;
        block[ "fields" ][ i ][ "value" ] = i;
        sceneJson[ m_blockName ] = JSON_ARRAY_( block );
        EXPECT_THROW( { const ActionBlocks::Actions actions = parse( sceneJson, m_blockType ); },
                      SceneEngine::SceneException );
    }
}

INSTANTIATE_TEST_CASE_P( ActionParserTest_DifferentBlockTypes,
                         ActionParserTest,
                         ::testing::Values( std::make_pair( "then", ScenesManager::ActionParser::BlockType::Then ),
                                            std::make_pair( "else", ScenesManager::ActionParser::BlockType::Else ) ) );

}  // namespace UT
}  // namespace ScenesManager
}  // namespace HubPlatform
