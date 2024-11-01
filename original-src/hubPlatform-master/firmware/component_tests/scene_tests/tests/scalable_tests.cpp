#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_type.h"
#include "database_client/dbtools.h"
#include "database_client/dbqueryparser.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1. Scalable values with isItemState
DONE            1.1 scalable non-temperature items without armed state
DONE            1.2 scalable non-temperature items without armed state and with no CoreD::Value default scale and with
                    no scale in when-block.
DONE    2. Scalable values with compareNumbers
DONE            2.1 scalable non-temperature items without armed state
DONE            2.2 scalable non-temperature items without armed state and with no CoreD::Value default scale and with
                    no scale in when-block.

*/

class SceneScalableTests : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

TEST_F( SceneScalableTests, isItemStateWithScale ) {  // 1.1
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Loudness, __LINE__ );
    std::string itemId = device.m_itemId;

    Json scene = Tests::createEmptyScene;
    // replace value with (value, scale) pair
    Json whenBlockWithScale = replaceTokens(
        Tests::isItemStateWhenBlock,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "float" }, { "__VALUE__", R"(50, "scale": "decibel")" } } );
    scene[ "params" ][ "when" ] = JSON_ARRAY_( whenBlockWithScale );

    SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, itemId, 50, __LINE__, "decibel" );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

    clearExistingDevices( __LINE__ );
}

TEST_F( SceneScalableTests, isItemStateNoScale ) {  // 1.2
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Loudness, __LINE__ );
    std::string itemId = device.m_itemId;

    Json scene = Tests::createEmptyScene;
    // replace value with (value) and no scale
    Json whenBlockNoScale = replaceTokens(
        Tests::isItemStateWhenBlock, { { "__ITEM_ID__", itemId }, { "__TYPE__", "float" }, { "__VALUE__", "50" } } );
    scene[ "params" ][ "when" ] = JSON_ARRAY_( whenBlockNoScale );

    SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, itemId, 50, __LINE__, "decibel" );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

    clearExistingDevices( __LINE__ );
}

TEST_F( SceneScalableTests, compareNumbersWithScale ) {  // 2.1
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Loudness, __LINE__ );
    std::string itemId = device.m_itemId;

    Json scene = Tests::createEmptyScene;
    // replace value with (value, scale) pair
    auto compareNumberBlock = replaceTokens( compareNumbersWhenBlock,
                                             { { "__COMPARATOR__", "==" },
                                               { "__TYPE__", "loudness" },
                                               { "__VALUE__", R"(50, "scale": "decibel")" },
                                               { "__SOURCE_NAME__", "item" },
                                               { "__SOURCE_TYPE__", "item" },
                                               { "__SOURCE_VALUE__", "\"" + itemId + "\"" } } );
    scene[ "params" ][ "when" ] = JSON_ARRAY_( compareNumberBlock );

    SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, itemId, 50, __LINE__, "decibel" );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

    clearExistingDevices( __LINE__ );
}

TEST_F( SceneScalableTests, compareNumbersNoScale ) {  // 2.2
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Loudness, __LINE__ );
    std::string itemId = device.m_itemId;

    Json scene = Tests::createEmptyScene;
    // replace value with (value) and no scale for loudness
    auto compareNumberBlock = replaceTokens( compareNumbersWhenBlock,
                                             { { "__COMPARATOR__", "==" },
                                               { "__TYPE__", "int" },
                                               { "__VALUE__", "50" },
                                               { "__SOURCE_NAME__", "item" },
                                               { "__SOURCE_TYPE__", "item" },
                                               { "__SOURCE_VALUE__", "\"" + itemId + "\"" } } );
    scene[ "params" ][ "when" ] = JSON_ARRAY_( compareNumberBlock );

    SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, itemId, 50, __LINE__, "decibel" );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

    clearExistingDevices( __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
