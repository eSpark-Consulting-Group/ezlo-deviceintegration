#include "scenes_manager/scenes_processor/parsers/scene_parser.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "common_utils/cpp14.h"
#include "service_interfaces/tests/fakes/fake_service.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace ScenesManager {
namespace UT {

class SceneParserTest : public testing::Test {
public:
    void SetUp() override {
        m_core = CoreDRemoteInterface::makeInstance( m_service );
        m_sceneParser = std::make_unique< ScenesManager::SceneParser >( *m_core );
    }
    SceneParserInterface::ScenePtr createScene( const JsonObject& sceneJson ) {
        return m_sceneParser->parse( sceneJson );
    }

private:
    Services::UT::FakeService m_service;
    std::unique_ptr< CoreDRemoteInterface::CoreDInterface > m_core;
    std::unique_ptr< ScenesManager::SceneParser > m_sceneParser;
};

TEST_F( SceneParserTest, sceneTest ) {
    SceneEngine::Scene::UserNotifications notifications = { "324234234", "456456453", "678678678" };
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "parent_id" ] = "76asdasd76yasf89";
    sceneJson[ "user_notifications" ] = JsonObject( notifications );
    auto scene = createScene( sceneJson );
    EXPECT_EQ( scene->getGroupId(), "" );
    EXPECT_EQ( scene->getName(), "EmptyName" );
    EXPECT_EQ( scene->getParentId(), "76asdasd76yasf89" );
    EXPECT_EQ( scene->getRoomName(), "" );
    EXPECT_EQ( scene->getUserNotifications(), notifications );
    EXPECT_EQ( scene->isEnabled(), true );
    EXPECT_EQ( scene->isGroup(), false );

    scene->addUserNotification( "1111111" );
    notifications.insert( "1111111" );
    EXPECT_EQ( scene->getUserNotifications(), notifications );
    scene->removeUserNotification( "456456453" );
    notifications.erase( "456456453" );
    EXPECT_EQ( scene->getUserNotifications(), notifications );
}

TEST_F( SceneParserTest, generatorFieldTest ) {
    SceneEngine::Scene::UserNotifications notifications = { "324234234", "456456453", "678678678" };
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "parent_id" ] = "76asdasd76yasf89";
    sceneJson[ "user_notifications" ] = JsonObject( notifications );
    sceneJson[ "generator" ] = JSON( "name"
                                     << "tcds_name"
                                     << "version"
                                     << "1234"
                                     << "role"
                                     << "system" );
    auto scene = createScene( sceneJson );
    EXPECT_EQ( scene->getGroupId(), "" );
    EXPECT_EQ( scene->getName(), "EmptyName" );
    EXPECT_EQ( scene->getParentId(), "76asdasd76yasf89" );
    EXPECT_EQ( scene->getRoomName(), "" );
    EXPECT_EQ( scene->getUserNotifications(), notifications );
    EXPECT_EQ( scene->isEnabled(), true );
    EXPECT_EQ( scene->isGroup(), false );

    auto generator = scene->getGeneratorData();
    EXPECT_EQ( generator[ "name" ], sceneJson[ "generator" ][ "name" ] );
    EXPECT_EQ( generator[ "version" ], sceneJson[ "generator" ][ "version" ] );
    EXPECT_EQ( generator[ "role" ], sceneJson[ "generator" ][ "role" ] );

    scene->addUserNotification( "1111111" );
    notifications.insert( "1111111" );
    EXPECT_EQ( scene->getUserNotifications(), notifications );
    scene->removeUserNotification( "456456453" );
    notifications.erase( "456456453" );
    EXPECT_EQ( scene->getUserNotifications(), notifications );
}

TEST_F( SceneParserTest, generatorFieldTestFailed ) {
    SceneEngine::Scene::UserNotifications notifications = { "324234234", "456456453", "678678678" };
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "parent_id" ] = "76asdasd76yasf89";
    sceneJson[ "user_notifications" ] = JsonObject( notifications );
    sceneJson[ "generator" ] = "test_generator";
    EXPECT_ANY_THROW( auto scene = createScene( sceneJson ) );

    sceneJson[ "generator" ] = 12345;
    EXPECT_ANY_THROW( auto scene = createScene( sceneJson ) );
}

}  // namespace UT
}  // namespace ScenesManager
}  // namespace HubPlatform
