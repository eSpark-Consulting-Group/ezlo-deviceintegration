#include "scenes_manager/scenes_engine/actions/scene_context_manager.h"

#include "scenes_manager/tests/resources/generic_scene_json.h"
#include "common_utils/cpp14.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"
#include <gtest/gtest.h>

using namespace SceneEngine;

namespace HubPlatform {
namespace ScenesManager {
namespace UT {

class SceneContextTest : public testing::Test {
public:
    IDbClient& getBDClient() {
        return m_service.getDBClient();
    }

private:
    Services::UT::FakeService m_service;
};

TEST_F( SceneContextTest, addContestActionsTest ) {
    SceneContextManager contextManager( getBDClient() );
    JsonObject message = JSON( "test"
                               << "test" );
    auto contextId = contextManager.createNewExecutionContext( "sceneId", 0, message );

    auto contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );

    EXPECT_EQ( contexts.size(), 1 );
    contextManager.addActonInfoToRun( contextId, "sceneId", "blockName", 1 );
    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 1 );
    auto context = contexts[ 0 ];

    EXPECT_EQ( json_get_str( context, "sceneId" ), "sceneId" );
    EXPECT_EQ( json_get_int( context, "timestamp" ), 0 );

    JsonArray actions;
    EXPECT_EQ( json_get_arr( context, "actions", actions ), true );
    EXPECT_EQ( actions.size(), 1 );

    contextManager.addActonInfoToRun( contextId, "sceneId", "blockName1", 2 );

    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    context = contexts[ 0 ];
    EXPECT_EQ( contexts.size(), 1 );
    EXPECT_EQ( json_get_arr( context, "actions", actions ), true );
    EXPECT_EQ( actions.size(), 2 );

    getBDClient().clear( HaTempDBSceneExecutionContextDBName );
}

TEST_F( SceneContextTest, removeContestActionsTest ) {
    SceneContextManager contextManager( getBDClient() );
    int action1 = 1;
    int action2 = 2;

    JsonObject message = JSON( "test"
                               << "test" );
    auto contextId1 = contextManager.createNewExecutionContext( "sceneId", 0, message );
    contextManager.addActonInfoToRun( contextId1, "sceneId", "blockName", action1 );
    contextManager.addActonInfoToRun( contextId1, "sceneId", "blockName1", action2 );

    auto contextId2 = contextManager.createNewExecutionContext( "sceneId", 0, message );
    contextManager.addActonInfoToRun( contextId2, "sceneId", "blockName", action1 );

    auto contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 2 );

    contextManager.removeActonFromRun( contextId1, action1 );
    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON( "_id" << contextId1 ), nullptr );
    EXPECT_EQ( contexts.size(), 1 );
    auto context = contexts[ 0 ];
    EXPECT_EQ( context[ "actions" ].size(), 1 );

    contextManager.removeActonFromRun( contextId1, action2 );
    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON( "_id" << contextId1 ), nullptr );
    EXPECT_EQ( contexts.size(), 1 );
    context = contexts[ 0 ];
    EXPECT_EQ( context[ "actions" ].size(), 0 );

    contextManager.removeContext( contextId1 );
    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON( "_id" << contextId1 ), nullptr );
    EXPECT_EQ( contexts.size(), 0 );
    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 1 );

    contextManager.removeContext( contextId2 );
    contexts = getBDClient().findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 0 );
}

}  // namespace UT
}  // namespace ScenesManager
}  // namespace HubPlatform
