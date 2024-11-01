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

DONE    1. Compare string

*/

class CompareStringsTests : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

TEST_F( CompareStringsTests, compareStrings ) /* 1 */ {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    std::string itemId = device.m_itemId;

    Json scene = Tests::createEmptyScene;
    auto comparisonBlock = replaceTokens( compareStringsWhenBlock,
                                          { { "__COMPARATOR__", "==" },
                                            { "__TYPE__", "string" },
                                            { "__VALUE__", "low" },
                                            { "__SOURCE_NAME__", "item" },
                                            { "__SOURCE_TYPE__", "item" },
                                            { "__SOURCE_VALUE__", "\"" + itemId + "\"" } } );

    scene[ "params" ][ "when" ] = JSON_ARRAY_( comparisonBlock );

    SceneID sceneId = createScene( scene );

    setItemValue( coreRemoteInterface, itemId, "low", __LINE__ );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

    clearExistingDevices( __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
