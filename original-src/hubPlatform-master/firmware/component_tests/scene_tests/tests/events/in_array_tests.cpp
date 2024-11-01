#include "scene_tests/tests/fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1. "inArray" operation for item source
DONE    1. "inArray" operation for expression source

*/

class InArrayTests : public ScenesExpressionsFixture {
public:
    void SetUp() override {
        ScenesExpressionsFixture::SetUp();
        callingFile( __FILE__ );
    }

    static JsonObject createInArrayWhenBlock( const std::string& operation,
                                              const std::string& sourceType,
                                              const std::string& sourceId,
                                              const std::string& type,
                                              const std::string& value ) {
        const KeyStringList replaceList = { { "__METHOD__", "inArray" },
                                            { "__OPERATION__", operation },
                                            { "__SOURCE_NAME__", sourceType },
                                            { "__SOURCE_TYPE__", sourceType },
                                            { "__SOURCE_VALUE__", "\"" + sourceId + "\"" },
                                            { "__TYPE__", type },
                                            { "__VALUE__", value } };
        return replaceTokens( itemOperationWhenBlock, replaceList );
    }
};

TEST_F( InArrayTests, inArrayOperationForItemSource ) {  // 1.
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    const std::string testItemId = device.m_itemId;

    const Json whenBlock = createInArrayWhenBlock( "in", "item", testItemId, "array.int", "[1, 2, 3]" );
    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( whenBlock );

    const SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, testItemId, Json( 1 ), __LINE__ );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );
    deleteScenes();
}

TEST_F( InArrayTests, inArrayOperationForExpressionSource ) {  // 2.
    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression =
        [this]( const std::string& name, int value, bool expressionExists, bool expectedBroadcast, int codeLine ) {
            auto code = "return " + std::to_string( value );
            scenesExpressionSetExt(
                name, code, {}, {}, expressionExists, "int", {}, false, expectedBroadcast, codeLine );
        };

    const int valueNotInArray = 0;
    const int valueInArray = 2;

    const std::string expressionSourceName = "expressionSource";
    setExpression( expressionSourceName, valueNotInArray, !expressionExists, expectBroadcast, __LINE__ );

    const Json whenBlock = createInArrayWhenBlock( "in", "expression", expressionSourceName, "array.int", "[1, 2, 3]" );
    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( whenBlock );

    const SceneID sceneId = createScene( scene );

    setExpression( expressionSourceName, valueInArray, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionSourceName ); } },
        __LINE__ );

    deleteScenes();
    deleteSceneExpression( expressionSourceName, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
