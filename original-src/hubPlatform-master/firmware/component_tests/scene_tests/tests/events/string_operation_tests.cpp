#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1. String operation with correct data

*/

class StringOperationTests : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }

    static JsonObject createStringOperationWhenBlock( const std::string& operation,
                                                      const std::string& sourceType,
                                                      const std::string& sourceId,
                                                      const std::string& type,
                                                      const std::string& value ) {
        const KeyStringList replaceList = { { "__METHOD__", "stringOperation" },
                                            { "__OPERATION__", operation },
                                            { "__SOURCE_NAME__", sourceType },
                                            { "__SOURCE_TYPE__", sourceType },
                                            { "__SOURCE_VALUE__", "\"" + sourceId + "\"" },
                                            { "__TYPE__", type },
                                            { "__VALUE__", value } };
        return replaceTokens( itemOperationWhenBlock, replaceList );
    }

    std::string testItemId;
};

TEST_F( StringOperationTests, stringOperationCorrectData ) {  // 1.
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    const std::string testItemId = device.m_itemId;

    const std::string value = "test_string";
    const Json whenBlock = createStringOperationWhenBlock( "begin", "item", testItemId, "string", '\"' + value + '\"' );
    Json scene = Tests::createEmptyScene;
    scene[ "params" ][ "when" ] = JSON_ARRAY_( whenBlock );

    const SceneID sceneId = createScene( scene );

    setItemValue( makeRemote(), testItemId, Json( value ), __LINE__ );

    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
