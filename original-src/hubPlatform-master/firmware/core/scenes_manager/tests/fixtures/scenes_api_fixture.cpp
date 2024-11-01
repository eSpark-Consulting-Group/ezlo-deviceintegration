#include "scenes_manager/tests/fixtures/scenes_api_fixture.h"
#include "scenes_manager/scenes_processor/scene_request_handler.h"
#include "scenes_manager/scenes_processor/scene_clone_manager.h"
#include "scenes_manager/scenes_engine/environment.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache.h"
#include "cored/group_operations/device_item_group_manager.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "scenes_manager/scenes_processor/scene_clone_manager.h"

#include "cored/remote_interface/cored_interface_factory.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"

namespace HubPlatform {
namespace UT {

ScenesAPIFixture::ScenesAPIFixture() = default;
ScenesAPIFixture::~ScenesAPIFixture() = default;
using KeyStringList = std::vector< std::pair< std::string, std::string > >;
void ScenesAPIFixture::SetUp() {
    m_service.reset( new ScenesManager::TestService() );

    auto sendBroadcast = [this]( MessageSubClasses, const JsonObject& message ) { m_result = message; };
    auto addTaskMethod = []( std::function< void() > func ) { func(); };

    m_core = CoreDRemoteInterface::makeInstance( *m_service );
    m_extensionsManager = std::make_unique< ExtensionsManager::RemoteInterface >( *m_service );
    m_extControllerInterface = std::make_unique< ExternalControllers::RemoteInterface >( *m_service );
    m_controllerFactory =
        std::make_unique< HubPlatform::ScenesManager::ControllerFactory >( *m_service, *m_extControllerInterface );

    m_itemCache = std::make_unique< SceneEngine::ItemCache >( *m_controllerFactory, m_service->getDBClient() );
    m_itemCache->setUp();

    m_expressions = std::make_unique< SceneEngine::Expressions::ExpressionManager >(
        *m_core, m_service->getDBClient(), *m_itemCache, "0000000" );
    m_expressions->setUp();

    m_clock = std::make_unique< SceneEngine::Clock >();
    m_statesRegistry = std::make_unique< SceneEngine::StatesRegistry >(
        m_service->getDBClient(), [this]() { return m_clock->greenwichMeanTimePoint(); } );

    m_scenesManager = std::make_unique< SceneEngine::SceneManager >( *m_service,
                                                                     *m_core,
                                                                     *m_controllerFactory,
                                                                     []( const JsonObject& ) {},
                                                                     *m_expressions,
                                                                     *m_itemCache,
                                                                     *m_deviceItemGroupManager,
                                                                     *m_statesRegistry,
                                                                     *m_clock );
    m_env = std::make_unique< SceneEngine::Environment >();
    m_scenesHandler = std::make_unique< ScenesManager::ScenesRequestHandler >( getService().getDBClient(),
                                                                               "",
                                                                               *m_scenesManager,
                                                                               *m_clock,
                                                                               *m_core,
                                                                               *m_extensionsManager,
                                                                               *m_expressions,
                                                                               *m_deviceItemGroupManager,
                                                                               sendBroadcast,
                                                                               addTaskMethod );

    m_sceneManagerRemoteInterface = std::make_unique< ScenesManager::RemoteInterface >( *m_service );

    m_service->setUp();

    cleanDB();

    {
        // Save setting in fake dbClient, don't try to write it via services sandwich
        const char* settingName = "scale.temperature";
        const char* settingValue = "celsius";
        m_service->getDBClient().insert( CoreD::Model::Resources::DBNames::SettingsDB,
                                         JSON( "_id" << settingName << "value" << settingValue ) );

        auto ptr = m_core->findSetting( "scale.temperature" );
        if( ptr )
            LOGGER_TRACE( ptr->getTokenValue() );
    }
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

void ScenesAPIFixture::TearDown() {
    cleanDB();
    m_service->tearDown();
    m_service.reset();
}

ScenesManager::Service& ScenesAPIFixture::getService() {
    return *m_service;
}

IDbClient& ScenesAPIFixture::getDbClient() {
    return m_service->getDBClient();
}

void ScenesAPIFixture::cleanDB() {
    getService().getDBClient().clean();
}

ScenesManager::RemoteInterface& ScenesAPIFixture::getSceneRemoteInterface() {
    return *m_sceneManagerRemoteInterface;
}

ScenesManager::ScenesRequestHandlerInterface& ScenesAPIFixture::getComplexScenesAPI() {
    return *m_scenesHandler;
}

SceneEngine::SceneManager& ScenesAPIFixture::getSceneManager() {
    return *m_scenesManager;
}

SceneEngine::SceneBlocks::ItemsToGet ScenesAPIFixture::convertToBlockItems(
    const std::vector< JsonObject >& dbItems,
    const std::vector< JsonObject >& dbOptionItems ) {
    return m_scenesHandler->convertToBlockItems( dbItems, dbOptionItems );
}

const JsonObject& ScenesAPIFixture::getResult() {
    return m_result;
}

std::string ScenesAPIFixture::getCodeLineString( int codeLine ) {
    return "code line: " + std::to_string( codeLine );
}

std::string ScenesAPIFixture::clonedSceneName( const std::string& name ) {
    Json scene = JSON_EMPTY_OBJ;
    auto scenesCloneManager = ScenesManager::ScenesCloneManager( getDbClient(), scene, *m_scenesManager );
    return scenesCloneManager.createClonedName( name );
}

}  // namespace UT
}  // namespace HubPlatform
