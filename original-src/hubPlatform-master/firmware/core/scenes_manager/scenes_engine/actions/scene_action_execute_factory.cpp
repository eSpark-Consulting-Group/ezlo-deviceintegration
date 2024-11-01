#include "scenes_manager/scenes_engine/actions/scene_action_execute_factory.h"
#include "scene_action_execute.h"

namespace SceneEngine {

SceneActionExecuteFactory::SceneActionExecuteFactory( HubPlatform::Services::Service& hostService,
                                                      HubPlatform::CoreDRemoteInterface::CoreDInterface& coreInterface,
                                                      ControllerFactoryInterface& controllerFactory,
                                                      BroadcastCallback sendBroadcast,
                                                      StatesRegistry& statesRegistry )
: m_hostService( hostService )
, m_coreInterface( coreInterface )
, m_controllerFactory( controllerFactory )
, m_sendBroadcast( std::move( sendBroadcast ) )
, m_stateRegistry( statesRegistry ) {}

SceneActionExecutePtr SceneActionExecuteFactory::create( ActionBlocks&& actions,
                                                         SceneSharedPtr scenePtr,
                                                         StatesRegistry& statesRegistry,
                                                         SceneContextManagerPtr sceneActionContextManager ) {
    return std::make_shared< SceneActionExecute >( std::move( actions ),
                                                   std::move( scenePtr ),
                                                   m_hostService,
                                                   m_coreInterface,
                                                   m_controllerFactory,
                                                   m_sendBroadcast,
                                                   m_stateRegistry,
                                                   sceneActionContextManager );
}

}  // namespace SceneEngine
