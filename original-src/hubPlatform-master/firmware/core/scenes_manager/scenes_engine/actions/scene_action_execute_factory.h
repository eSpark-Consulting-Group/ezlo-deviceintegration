#pragma once
#include "scenes_manager/scenes_engine/actions/iscene_action_execute.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/actions/scene_context_info.h"

namespace HubPlatform {
namespace Services {
class Service;
}
namespace CoreDRemoteInterface {
class CoreDInterface;
}
}  // namespace HubPlatform

namespace SceneEngine {

class ControllerFactoryInterface;

class SceneActionExecuteFactory {
public:
    SceneActionExecuteFactory( HubPlatform::Services::Service& hostService,
                               HubPlatform::CoreDRemoteInterface::CoreDInterface& coreInterface,
                               ControllerFactoryInterface& controllerFactory,
                               BroadcastCallback sendBroadcast,
                               StatesRegistry& statesRegistry );

    SceneActionExecutePtr create( ActionBlocks&& actions,
                                  SceneSharedPtr scenePtr,
                                  StatesRegistry& statesRegistry,
                                  SceneContextManagerPtr sceneActionContextManager );

private:
    HubPlatform::Services::Service& m_hostService;
    HubPlatform::CoreDRemoteInterface::CoreDInterface& m_coreInterface;
    ControllerFactoryInterface& m_controllerFactory;
    BroadcastCallback m_sendBroadcast;
    StatesRegistry& m_stateRegistry;
};

}  // namespace SceneEngine