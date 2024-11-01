#include "scenes_manager/tests/fakes/test_scenes_service.h"
#include "scenes_manager/scenes_engine/scene_manager.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"

#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "scenes_manager/scenes_processor/controller_interface/controller_factory.h"

#include "test_utils/temporary_root_folder_wrapper.h"
#include "base_utils/utils.h"

#include "gtest/gtest.h"

namespace HubPlatform {
namespace ScenesManager {
class ScenesRequestHandler;
class ScenesCloneManager;
}  // namespace ScenesManager

namespace UT {

class ScenesAPIFixture : public ::testing::Test {
protected:
    ScenesAPIFixture();
    ~ScenesAPIFixture();

    void SetUp() override;
    void TearDown() override;

    ScenesManager::Service& getService();
    IDbClient& getDbClient();
    void cleanDB();

    ScenesManager::RemoteInterface& getSceneRemoteInterface();
    ScenesManager::ScenesRequestHandlerInterface& getComplexScenesAPI();
    SceneEngine::SceneManager& getSceneManager();
    SceneEngine::SceneBlocks::ItemsToGet convertToBlockItems( const std::vector< JsonObject >& dbItems,
                                                              const std::vector< JsonObject >& dbOptionItems );
    std::string clonedSceneName( const std::string& name );

    const JsonObject& getResult();

    using KeyStringList = std::vector< std::pair< std::string, std::string > >;
    static std::string replaceTokens( std::string source, const KeyStringList& keys ) {
        for( const auto& it : keys ) {
            replaceAll( source, it.first, it.second );
        };
        return source;
    }

    std::string getCodeLineString( int codeLine );

protected:
    Tests::TemporaryRootFolderWrapper m_rootFolderWrapper;

    // Service already own the complexOpApi
    std::unique_ptr< SceneEngine::IClock > m_clock;
    std::unique_ptr< HubPlatform::ScenesManager::TestService > m_service;
    std::unique_ptr< HubPlatform::ScenesManager::ScenesRequestHandler > m_scenesHandler;
    CoreDRemoteInterface::CoreDInterfaceUPtr m_core;
    std::unique_ptr< ExtensionsManager::RemoteInterface > m_extensionsManager;
    std::unique_ptr< ExternalControllers::RemoteInterface > m_extControllerInterface;
    std::unique_ptr< HubPlatform::ScenesManager::ControllerFactory > m_controllerFactory;
    std::unique_ptr< SceneEngine::ItemCacheBase > m_itemCache;
    std::unique_ptr< SceneEngine::StatesRegistry > m_statesRegistry;
    std::unique_ptr< SceneEngine::Expressions::ExpressionManager > m_expressions;
    std::unique_ptr< CoreD::DeviceItemGroupManager > m_deviceItemGroupManager;
    std::unique_ptr< SceneEngine::SceneManager > m_scenesManager;
    std::unique_ptr< SceneEngine::Environment > m_env;
    std::unique_ptr< ScenesManager::RemoteInterface > m_sceneManagerRemoteInterface;
    JsonObject m_result;
};

}  // namespace UT
}  // namespace HubPlatform
