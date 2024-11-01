#pragma once
// Extracted from action.h/cpp
#include "scenes_manager/scenes_engine/actions/iaction_executor.h"
#include "scene_context_info.h"
#include "scenes_manager/scenes_engine/actions/scene_context_info.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace SceneEngine {

class ActionExecutor : public IActionExecutor {
public:
    ActionExecutor( HubPlatform::Services::Service& hostService,
                    std::string sceneId,
                    BroadcastCallback sendBroadcast,
                    JsonObject broadcast,
                    SceneContextManagerPtr contextInfos,
                    std::string contextId );
    ~ActionExecutor();
    bool setResult( const ActionResult& result ) override;
    void setActions( const ActionList& actions ) override;
    void setActions( ActionList&& actions ) override;
    void setSceneId( std::string sceneId ) override;
    const std::string& getSceneId() const override;
    void runNextAction( std::string contextId ) override;

private:
    HubPlatform::Services::Service& m_hostService;
    BroadcastCallback m_sendBroadcast;
    JsonObject m_broadcast;
    ActionList m_actions;
    ResultList m_resultList;
    std::size_t m_actionQuantity;
    std::string m_sceneId;
    SceneContextManagerPtr m_contextManager;
    int m_taskPosition = 0;
    std::string m_contextId;
};

}  // namespace SceneEngine
