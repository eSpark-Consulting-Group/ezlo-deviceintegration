#pragma once
#include "json_utils/json_fwd.h"
#include "scenes_manager/scenes_processor/actions/action_interface.h"
#include <chrono>

enum MessageSubClasses : int;

namespace SceneEngine {
class ControllerFactoryInterface;
class IClock;
}  // namespace SceneEngine

namespace HubPlatform {

namespace Services {
class Service;
}

namespace ScenesManager {

class ScenesRequestHandlerInterface {
public:
    using AddTask = std::function< void( std::function< void() > ) >;
    using ControllerId = std::string;
    using Callback = std::function< void( const Json& response ) >;

    using BroadcastCallback = std::function< void( MessageSubClasses messageSubClass, const JsonObject message ) >;
    using ServiceTask = std::function< void() >;
    using AddServiceTask = std::function< void( ServiceTask ) >;

public:
    virtual ~ScenesRequestHandlerInterface() = default;
    virtual void setUp() = 0;
    virtual void tearDown() = 0;

    virtual bool processCreateScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processEditScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processDeleteScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processDeleteAllScenes( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processEnableScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processCloneScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processRunScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processStopScene( const JsonObject& params, JsonObject& answer ) = 0;
    virtual bool processSetRoom( const JsonObject& params, JsonObject& answer ) = 0;

    virtual JsonObject processRoomWasRemoved( const JsonObject& params ) = 0;
    virtual JsonObject processAddNotification( const JsonObject& params ) = 0;
    virtual JsonObject processRemoveNotification( const JsonObject& params ) = 0;
    virtual JsonObject processTimeList( const JsonObject& params ) = 0;
    virtual JsonObject processSetBlocks( const JsonObject& params ) = 0;
    virtual JsonObject processSetIcon( const JsonObject& params ) = 0;
    virtual JsonObject processRemoveBlocks( const JsonObject& params ) = 0;
    virtual JsonObject processGetBlocks( const JsonObject& params ) = 0;
    virtual JsonObject processGetBlocksDataList( const JsonObject& params ) = 0;
    virtual JsonObject processBlockStatusReset( const JsonObject& params ) = 0;
    virtual JsonObject processScenesStatusGet( const JsonObject& params ) = 0;
    virtual JsonObject modifyScenes( const JsonObject& params ) = 0;
    virtual JsonObject getExpressionValue( const JsonObject& params ) = 0;
    virtual JsonObject setSceneName( const JsonObject& params ) = 0;
    virtual JsonObject setHouseModes( const JsonObject& params ) = 0;
    virtual JsonObject setBlockEnabled( const JsonObject& params ) = 0;
    virtual JsonObject setSceneMeta( const JsonObject& params ) = 0;
    virtual JsonObject setSceneBlockMeta( const JsonObject& params ) = 0;

    virtual void actionBlockTest( const JsonObject& params,
                                  Services::Service& hostService,
                                  SceneEngine::ControllerFactoryInterface& controllerFactory,
                                  Callback callback ) = 0;
    virtual void loadScenes() = 0;
    virtual void setClock( SceneEngine::IClock& clock ) = 0;

    virtual void itemAddedHandler( const JsonObject& message ) = 0;
    virtual void itemRemovedHandler( const JsonObject& message ) = 0;
    virtual void itemUpdatedHandler( const JsonObject& message ) = 0;
    virtual void deviceUpdatedHandler( const JsonObject& message ) = 0;
    virtual void deviceRemovedHandler( const JsonObject& message ) = 0;

    virtual JsonObject getSceneById( const JsonObject& message ) = 0;
    virtual JsonArray getScenesList( const JsonObject& message ) = 0;
    virtual JsonArray getGlobalScenesList( const JsonObject& message ) = 0;

    virtual JsonObject expressionSet( const JsonObject& message ) = 0;
    virtual JsonObject expressionDelete( const JsonObject& message ) = 0;
    virtual JsonObject expressionList( const JsonObject& message ) = 0;
    virtual void saveHubBackupRunCallbackPtr( const ActionInterface::Action& action,
                                              ActionInterface::ActionExecutorPtr actionExecutorPtr ) = 0;
    virtual void setHubBackupRunState( bool flag ) = 0;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
