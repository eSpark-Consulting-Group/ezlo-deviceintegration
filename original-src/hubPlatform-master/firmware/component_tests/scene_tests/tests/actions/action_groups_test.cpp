#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**
 * @case_01 Unused action group
 * @case_02 Action group linked to logical operator
 * @case_03 Action group linked to event operator (isItemState)
 */

namespace HubPlatform {
namespace Tests {

class SceneActionBlockTest_actionGroups : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

/// @case_01
/// @brief
/// The test verifies validation of action groups
/// The case creates a scene with an action group not linked to any when block
/// An error is expected
TEST_F( SceneActionBlockTest_actionGroups, unusedActionGroup ) {  // 1.56.1
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto request = createEmptyScene;

    request[ "params" ][ "thenGroups" ][ "random_id" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );

    auto error = nma().sendtoHubAndGetAnswer( request )[ "error" ];

    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.groups.unused" );
}

/// @case_02
/// @brief
/// The test verifies behaviour of action groups with a logical operator (OR in this case)
/// An action block linked to a specific when block must be executed when the condition is triggered
TEST_F( SceneActionBlockTest_actionGroups, logicalOperator ) {  // 1.56.2
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto dev2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto request = createEmptyScene;

    auto whenBlock = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );
    whenBlock.append( "," ).append( replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "20" } } ) );
    auto orOperator = replaceTokens( orOperatorWhenBlock, { { "__BLOCKS__", std::move( whenBlock ) } } );

    std::string actionGroupIdOfOrOperator = "0";

    // Action group will be linked to this OR operator
    orOperator[ "actionGroup" ] = actionGroupIdOfOrOperator;

    request[ "params" ][ "when" ].push_back( orOperator );

    request[ "params" ][ "then" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev2.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "11" } } ) );

    // Even if the OR condition is false, we will execute this action block because it is linked to FALSE state of OR
    request[ "params" ][ "elseGroups" ][ actionGroupIdOfOrOperator ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev2.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    auto& coreRemoteInterface = makeRemote( nma() );

    // This won't match any condition, thus elseGroup actions linked to the OR operator should be executed
    setItemValue( coreRemoteInterface, dev1.m_itemId, 15, __LINE__ );

    // Expect that elseGroups["0"] is executed
    checkItemUpdateBroadcast( dev2.m_itemId, 12, __LINE__ );

    // Not expecting any progress broadcasts because they must be sent only for "then" block
}

/// @case_03
/// @brief
/// The test verifies behaviour of action groups with an event operator (isItemState in this case)
/// An action block linked to a specific when block must be executed when the condition is triggered
TEST_F( SceneActionBlockTest_actionGroups, eventOperator ) {  // 1.56.3
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto dev2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto dev3 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto request = createEmptyScene;

    std::string actionGroupIdOfWhenBlock1 = "0";

    auto whenBlock1 = replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );

    // Action group will be linked to this when block
    whenBlock1[ "actionGroup" ] = actionGroupIdOfWhenBlock1;

    auto whenBlock2 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev2.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "20" } } );

    auto andOperator = replaceTokens(
        andOperatorWhenBlock, { { "__BLOCKS__", std::move( whenBlock1.dump().append( "," ).append( whenBlock2 ) ) } } );

    request[ "params" ][ "when" ].push_back( andOperator );

    request[ "params" ][ "then" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev3.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "11" } } ) );

    // Even if the AND condition is false, we will execute this action block if whenBlock1 becomes true
    request[ "params" ][ "thenGroups" ][ actionGroupIdOfWhenBlock1 ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev3.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );

    SceneID sceneId = createScene( request );

    auto& coreRemoteInterface = makeRemote( nma() );

    // This won't match the condition of AND operator, but one block-operand will evaluate to TRUE
    setItemValue( coreRemoteInterface, dev1.m_itemId, 10, __LINE__ );

    // So we expect that a corresponding then group will be executed
    checkItemUpdateBroadcast( dev3.m_itemId, 12, __LINE__ );

    // Not expecting any progress broadcasts because they must be sent only for "then" block
}

}  // namespace Tests
}  // namespace HubPlatform
