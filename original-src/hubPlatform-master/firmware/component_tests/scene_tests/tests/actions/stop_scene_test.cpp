#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**
 * TESTPLAN
 * @case_01 Request to stop "then" block
 * @case_02 Request to stop "thenGroups" block
 * @case_03 Invalid "sceneId" field value
 * @case_04 Invalid action group ID
 */

namespace HubPlatform {
namespace Tests {

class StopSceneActionTest : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

/// @case_01
/// @brief
/// stopScene action is a method of action block
/// Stops scene by its id
/// The test verifies a valid request to stop then block
TEST_F( StopSceneActionTest, thenBlock ) {
    auto request =
        replaceTokens( createSceneRequest,
                       { { "__NAME__", "scenesStopSuccess" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    // Todo: call stopScene action instead of deleteScene function and verify
    deleteScene( sceneId );
}

}  // namespace Tests
}  // namespace HubPlatform
