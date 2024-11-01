#include "fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "test_utils/test_utils.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE            1.1 Compare numbers range  (comparator between)
DONE            1.2 Compare numbers range invalid args (comparator between)
DONE            1.3 Compare numbers range (comparator not_between)
DONE            1.4 Compare numbers range invalid args (comparator not_between)
DONE            1.5 Compare numbers range for expression source
*/

class SceneNumberRangeTests : public ScenesExpressionsFixture {
public:
    void SetUp() override {
        ScenesExpressionsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

TEST_F( SceneNumberRangeTests, compareNumbersRangeBetween ) {  // 1.1
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( device.m_itemId, 1, __LINE__ );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( device.m_itemId ) },
                                          { "__COMP__", "between" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbesRangeBetween" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 50 ) );
    checkItemUpdateBroadcast( device.m_itemId, 50, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScenes();

    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( device.m_deviceId ).size();
    removeDevice( coreRemoteInterface, device.m_deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( SceneNumberRangeTests, compareNumbersRange_betweenInvalidArgs ) {  // 1.2
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( device.m_itemId, 1, __LINE__ );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( device.m_itemId ) },
                                          { "__COMP__", "between" } } );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "compareNumbesRangeBetweenInvalidArgs" },
                                    { "__WHEN_BLOCKS__", block },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 9 ) );
    checkItemUpdateBroadcast( device.m_itemId, 9, __LINE__ );

    deleteScenes();

    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( device.m_deviceId ).size();
    removeDevice( coreRemoteInterface, device.m_deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( SceneNumberRangeTests, compareNumbersRangeNotBetween ) {  // 1.3
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( device.m_itemId, 10, __LINE__ );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( device.m_itemId ) },
                                          { "__COMP__", "not_between" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbesRangeNotBetween" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( device.m_itemId, 2, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScenes();

    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( device.m_deviceId ).size();
    removeDevice( coreRemoteInterface, device.m_deviceId, itemsNumber, 0, __LINE__ );
    // sleep(100);
}

TEST_F( SceneNumberRangeTests, compareDataRange1_notBetweenInvalidArgs ) {  // 1.4
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 20 ) );
    checkItemUpdateBroadcast( device.m_itemId, 20, __LINE__ );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( device.m_itemId ) },
                                          { "__COMP__", "not_between" } } );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "compareNumbesRangeNotBetweenInvalidArgs" },
                                    { "__WHEN_BLOCKS__", block },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << device.m_itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( device.m_itemId, 10, __LINE__ );

    deleteScenes();

    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( device.m_deviceId ).size();
    removeDevice( coreRemoteInterface, device.m_deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( SceneNumberRangeTests, compareNumbersRangeForExpressionSource ) {  // 1.5
    using namespace CoreD::Model::Resources;

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression =
        [this]( const std::string& name, int value, bool expressionExists, bool expectedBroadcast, int codeLine ) {
            auto code = "return " + std::to_string( value );
            scenesExpressionSetExt(
                name, code, {}, {}, expressionExists, "int", {}, false, expectedBroadcast, codeLine );
        };

    const int valueNotBetween = 1;
    const int valueBetween = 25;

    const std::string expressionSourceName = "expressionSource";
    setExpression( expressionSourceName, valueNotBetween, !expressionExists, expectBroadcast, __LINE__ );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "expression" },
                                          { "__SOURCE_TYPE__", "expression" },
                                          { "__SOURCE_VALUE__", quote( expressionSourceName ) },
                                          { "__COMP__", "between" } } );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "compareNumbesRangeForExpressionSource" },
                                    { "__WHEN_BLOCKS__", block },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request );

    setExpression( expressionSourceName, valueBetween, expressionExists, !expectBroadcast, __LINE__ );

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
