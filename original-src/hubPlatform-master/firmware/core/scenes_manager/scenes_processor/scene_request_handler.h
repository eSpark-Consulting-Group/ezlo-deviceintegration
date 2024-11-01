#pragma once
#include "scenes_manager/scenes_processor/scene_request_handler_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "scenes_manager/scenes_processor/parsers/scene_parser.h"
#include "scenes_manager/scenes_processor/parsers/action_parser.h"
#include "scenes_manager/scenes_processor/parsers/event_parser_interface.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/scene.h"
#include "scenes_manager/scenes_engine/scene_blocks.h"
#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_path.h"
#include "scenes_manager/scenes_processor/anonymous_plugin_operations.h"
#include "cored/group_operations/device_item_group_manager.h"
#include "cored/group_operations/group.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/value_source_key.h"
#include "scenes_manager/scenes_processor/actions/action_interface.h"

class IDbClient;

namespace SceneEngine {
class SceneManager;
class IClock;
class EventContainer;
}  // namespace SceneEngine

namespace HubPlatform {

namespace UT {
class ScenesAPIFixture;
}  // namespace UT

namespace ScenesManager {

class ScenesRequestHandler : public ScenesRequestHandlerInterface {
public:
    using BroadcastCallback = std::function< void( MessageSubClasses messageSubClass, const JsonObject message ) >;
    using ServiceTask = std::function< void() >;
    using AddServiceTask = std::function< void( ServiceTask ) >;

public:
    ScenesRequestHandler( IDbClient& client,
                          const ControllerId& controllerId,
                          SceneEngine::SceneManager& sceneManager,
                          SceneEngine::IClock& clock,
                          CoreDRemoteInterface::CoreDInterface& coreInterface,
                          ExtensionsManager::RemoteInterface& extensionsManagerInterface,
                          SceneEngine::Expressions::ExpressionManager& expressions,
                          CoreD::DeviceItemGroupManager& deviceItemGroupManager,
                          BroadcastCallback sendBroadcast,
                          AddServiceTask addServiceTask );
    ~ScenesRequestHandler() override = default;
    void setUp() override;
    void tearDown() override;
    bool processCreateScene( const JsonObject& params, JsonObject& answer ) override;
    bool processEditScene( const JsonObject& params, JsonObject& answer ) override;
    bool processDeleteScene( const JsonObject& params, JsonObject& answer ) override;
    bool processDeleteAllScenes( const JsonObject& params, JsonObject& answer ) override;
    bool processEnableScene( const JsonObject& params, JsonObject& answer ) override;
    bool processCloneScene( const JsonObject& params, JsonObject& answer ) override;
    bool processRunScene( const JsonObject& params, JsonObject& answer ) override;
    bool processStopScene( const JsonObject& params, JsonObject& answer ) override;
    bool processSetRoom( const JsonObject& params, JsonObject& answer ) override;
    JsonObject processRoomWasRemoved( const JsonObject& params ) override;
    JsonObject processAddNotification( const JsonObject& params ) override;
    JsonObject processRemoveNotification( const JsonObject& params ) override;
    JsonObject processTimeList( const JsonObject& params ) override;
    /* @params sourceId - id of blocks source */
    JsonObject processSetBlocks( const JsonObject& params ) override;
    /* @params id - id of scene
     * @params icon - scene icon
     */
    JsonObject processSetIcon( const JsonObject& params ) override;
    /* @params sourceId - id of blocks source */
    JsonObject processRemoveBlocks( const JsonObject& params ) override;
    /* @params devices - array of devices ids */
    JsonObject processGetBlocks( const JsonObject& params ) override;
    JsonObject processGetBlocksDataList( const JsonObject& params ) override;
    JsonObject processBlockStatusReset( const JsonObject& params ) override;
    JsonObject processScenesStatusGet( const JsonObject& params ) override;
    JsonObject modifyScenes( const JsonObject& params ) override;
    JsonObject getExpressionValue( const JsonObject& params ) override;
    JsonObject setSceneName( const JsonObject& params ) override;
    JsonObject setHouseModes( const JsonObject& params ) override;
    JsonObject setBlockEnabled( const JsonObject& params ) override;
    JsonObject setSceneMeta( const JsonObject& params ) override;
    JsonObject setSceneBlockMeta( const JsonObject& params ) override;
    void actionBlockTest( const JsonObject& params,
                          Services::Service& hostService,
                          SceneEngine::ControllerFactoryInterface& controllerFactory,
                          Callback callback ) override;
    void loadScenes() override;
    void setClock( SceneEngine::IClock& clock ) override;
    void itemAddedHandler( const JsonObject& message ) override;
    void itemRemovedHandler( const JsonObject& message ) override;
    void itemUpdatedHandler( const JsonObject& message ) override;
    void deviceUpdatedHandler( const JsonObject& message ) override;
    void deviceRemovedHandler( const JsonObject& message ) override;

    JsonObject getSceneById( const JsonObject& message ) override;
    JsonArray getScenesList( const JsonObject& message ) override;
    JsonArray getGlobalScenesList( const JsonObject& message ) override;

    JsonObject expressionSet( const JsonObject& message ) override;
    JsonObject expressionDelete( const JsonObject& message ) override;
    JsonObject expressionList( const JsonObject& message ) override;
    void saveHubBackupRunCallbackPtr( const ActionInterface::Action& action,
                                      ActionInterface::ActionExecutorPtr actionExecutorPtr ) override;
    void setHubBackupRunState( bool flag ) override;

protected:
    static void wrongParam( const std::string& fieldName, JsonObject& answer );
    JsonObject updateSceneParam( const SceneEngine::Scene::SceneID& sceneId,
                                 const std::string& fieldName,
                                 const Json& value,
                                 bool isGlobal = false );
    JsonObject cloneScene( const SceneEngine::Scene::SceneID& sceneId, const std::string& sceneName, bool isGlobal );
    JsonObject enableSceneBlock( const SceneEngine::Scene::SceneID& sceneId,
                                 const std::string& blockId,
                                 bool enabled,
                                 bool isGlobal = false );

    JsonObject* findNestedBlock( Json& block, const std::string& blockId );
    SceneEngine::SceneBlocks::ItemsToGet convertToBlockItems( const std::vector< JsonObject >& dbItems,
                                                              const std::vector< JsonObject >& dbOptionItems );
    bool isInvalidValueRange( const std::string& fieldName,
                              const std::string& field,
                              std::size_t maxSize,
                              JsonObject& answer ) const;
    std::string fetchTemperatureScale() const;

    std::pair< bool, Json > prepareCreateSafe( const Json& scene );
    std::pair< bool, Json > prepareCreate( const Json& scene );
    std::pair< bool, Json > prepareCreate( const Json& scene, const std::string& defaultScale );
    std::pair< bool, Json > prepareFetch( const Json& scene, const std::string& defaultScale );

    void moveScenesWithIsSceneStateWhenBlockToBottom( std::vector< Json >& scenes );
    IDbClient& getDBClient();
    const IDbClient& getDBClient() const;

    JsonObject doExpressionSet( const JsonObject& message );
    JsonObject doVariableExpressionSet( const JsonObject& message );
    bool cloneScene( const SceneEngine::Scene::SceneID& sceneId,
                     JsonObject& clonedScene,
                     const std::string& sceneName );

    using ActionGroupMappings = std::map< std::string, std::string >;
    SceneEngine::ActionBlocks parseActions( JsonObject& scene,
                                            IdUtils::PlaceholdersMapping& placeholdersMapping,
                                            const ActionGroupMappings& mappings );
    void fillBlocksSources( const IDbClient& dbClient );

    void checkEventsWithExpressions( const SceneEngine::EventContainer& events );

    void safeCreateScene( JsonObject& scene );
    void createSceneInternal( SceneParserInterface::ScenePtr& scenePtr, JsonObject& scene, bool isEditing );

    using ItemEventBaseEvent = std::pair< SceneEngine::ValueSourceKey, SceneEngine::ItemEventBasePtr >;
    void checkEvents( const std::vector< ItemEventBaseEvent >& events );
    void checkEvent( const SceneEngine::ItemEventBasePtr& itemEvent, const SceneEngine::ValueSourceKey& searchKey );
    using ItemEvent = std::pair< SceneEngine::ValueSourceKey, SceneEngine::ItemEventPtr >;
    using ItemEvents = std::vector< ItemEvent >;
    void checkItemEvents( const ItemEvents& events );
    void checkItemEvent( const SceneEngine::ItemEventPtr& itemEvent, const SceneEngine::ValueSourceKey& searchKey );
    void addEmptyScene( SceneParserInterface::ScenePtr& scenePtr, const JsonObject& scene );
    static void setSceneError( SceneParserInterface::ScenePtr& scenePtr, const std::string& errorMessage );
    void updatePluginsAcquiredCountForScene( Json& scene );
    std::string createClonedName( const std::string& name, const IDbClient& dbClient );

private:
    bool checkScenesListParams( const JsonObject& params );

protected:
    IDbClient& m_dbClient;
    SceneEngine::SceneManager& m_manager;
    // Do not use (const IDbClient &) from core interface !!!
    CoreDRemoteInterface::CoreDInterface& m_core;
    ExtensionsManager::RemoteInterface& m_extensionsManager;
    std::unique_ptr< SceneParserInterface > m_sceneParser;
    std::unique_ptr< ActionParserInterface > m_actionParser;
    std::unique_ptr< EventParserInterface > m_eventParser;
    std::unique_ptr< AnonymousPluginOperations > m_anonymousPluginOperations;
    std::reference_wrapper< SceneEngine::IClock > m_clock;
    SceneEngine::Expressions::ExpressionManager& m_expressions;
    CoreD::DeviceItemGroupManager& m_deviceItemGroupManager;
    BroadcastCallback m_sendBroadcast;
    AddServiceTask m_addServiceTask;
    SceneEngine::SceneBlocks m_blocks;

    friend class UT::ScenesAPIFixture;

    void expressionUpdate( const std::string& name,
                           const std::string* valueType,
                           const Json* metadata,
                           const Json* value,
                           const bool* variable,
                           bool setVariable,
                           const Json& prevValue,
                           SceneEngine::Expressions::TypedVariableExpressionUPtr& expressionPtr,
                           const Json* error );

    void sendChangedBroadcast( const std::string& name,
                               const std::string* valueType,
                               const Json* metadata,
                               const Json* value,
                               const bool* variable,
                               const Json* error );
};

}  // namespace ScenesManager
}  // namespace HubPlatform
