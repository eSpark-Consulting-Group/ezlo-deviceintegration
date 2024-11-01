#pragma once

#include "scenes_manager/scenes_engine/actions/iscene_action_execute.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/task_loop_timer.h"
#include "service_interfaces/task_loop.h"
#include "scenes_manager/scenes_engine/actions/scene_context_manager.h"
#include "scene_context_info.h"
#include "scenes_manager/scenes_engine/states_registry.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace SceneEngine {

class SceneActionExecute : public ISceneActionExecute {
public:
    SceneActionExecute( SceneEngine::ActionBlocks&& actions,
                        SceneSharedPtr scenePtr,
                        HubPlatform::Services::Service& hostService,
                        HubPlatform::CoreDRemoteInterface::CoreDInterface& coreInterface,
                        ControllerFactoryInterface& controllerFactory,
                        BroadcastCallback sendBroadcast,
                        StatesRegistry& statesRegistry,
                        SceneContextManagerPtr sceneActionContextManager );

    SceneActionExecute( const SceneActionExecute& ) = delete;
    SceneActionExecute( SceneActionExecute&& ) = delete;
    ~SceneActionExecute();

    virtual bool isEnabled() const;

    /**
     * @brief
     * Run "then" block actions, usually invoked when full scene condition becomes TRUE
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runThenActions( const std::string& contextId ) const;

    /**
     * @brief
     * Run actions linked to the TRUE condition of a given "when" block (i.e. "thenGroups" actions)
     * Usually invoked when a corresponding scene operator is triggered and its state becomes TRUE
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runThenActions( const std::string& whenBlockID, const std::string& contextId ) const;

    /**
     * @brief
     * Run "else" block actions, usually invoked when full scene condition becomes FALSE
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runElseActions( const std::string& contextId ) const;

    /**
     * @brief
     * Run actions linked to the FALSE condition of a given "when" block (i.e. "elseGroups" actions)
     * Usually invoked when a corresponding scene operator is triggered and its state becomes TRUE
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runElseActions( const std::string& whenBlockID, const std::string& contextId ) const;

    virtual SceneSharedPtr getScene() const;
    virtual Scene::SceneID getSceneId() const;
    virtual ActionBlocks::Actions getActions( const std::string& name ) const;
    virtual ActionBlocks::Actions getThenActions( const std::string& name ) const;
    virtual bool hasThenActions() const;
    virtual bool hasElseActions() const;
    virtual void stopScene( const std::string& sceneId, const RunSceneActionBlockType blockType );

private:
    void initializeAction( ActionSharedPtr );
    void deinitializeAction( ActionSharedPtr );
    bool isHouseModeAllowed() const;
    JsonObject getBroadcastMessage( const std::string& sceneStatus ) const;
    JsonObject getGlobalBroadcastMessage( const std::string& sceneStatus ) const;
    void sendProgressBroadcast( const std::string& status ) const;
    bool runActions( const ActionBlocks::Actions& actions,
                     bool sendBroadcasts,
                     const std::string& blockName,
                     const std::string& contextId ) const;
    void runDelayedActions( const ActionBlocks::Actions& actions, const std::string& contextId ) const;
    bool getGlobalRestrictionState() const;

private:
    SceneSharedPtr m_scenePtr;
    SceneEngine::ActionBlocks m_actions;
    HubPlatform::Services::Service& m_hostService;
    HubPlatform::CoreDRemoteInterface::CoreDInterface& m_coreInterface;
    ControllerFactoryInterface& m_controllerFactory;
    BroadcastCallback m_sendBroadcast;
    SceneContextManagerPtr m_sceneContextManager;
    StatesRegistry& m_statesRegistry;
};

}  // namespace SceneEngine
