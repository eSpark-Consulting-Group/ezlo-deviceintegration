#pragma once
#include "scenes_manager/scenes_processor/actions/scene_actions.h"

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace ScenesManager {

class GroupToggleValueAction : public ControllerCoreActions {
public:
    GroupToggleValueAction( ControllerFactory& controllerFactory, Services::Service& m_hostService );
    void run( const Action& action, ActionExecutorPtr actionExecutorPtr ) override;

private:
    Services::Service& m_hostService;
};

}  // namespace ScenesManager
}  // namespace HubPlatform