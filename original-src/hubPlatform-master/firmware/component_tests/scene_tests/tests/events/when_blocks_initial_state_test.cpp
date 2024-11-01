#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1. houseModeChangedTo

*/

class WhenBlocksInitialStateTest : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

TEST_F( WhenBlocksInitialStateTest, houseModeChangedTo ) {  // 1.
    auto& coreRemoteInterface = makeRemote( nma() );

    auto whenBlock = replaceTokensInString( isHouseModeChangedToWhenBlock, { { "__HOUSE_MODES__", quote( "1" ) } } );
    auto scene = replaceTokens( createSceneWithExceptionsRequest,
                                { { "__NAME__", "houseModeChangedTo_init" },
                                  { "__THEN_BLOCKS__", "" },
                                  { "__WHEN_BLOCKS__", "" },
                                  { "__EXCEPTION_BLOCKS__", whenBlock } } );

    const SceneID sceneId = createScene( scene, __LINE__ );
    const Json sceneStatus = getSceneStatus( sceneId, __LINE__ );

    std::string state;
    std::vector< Json > keyPath{ "result", "status", "exceptions", 0, "state" };
    ASSERT_TRUE( json_get( sceneStatus, keyPath, state ) );
    EXPECT_EQ( state, "true" );
}

}  // namespace Tests
}  // namespace HubPlatform
