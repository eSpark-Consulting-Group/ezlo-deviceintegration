#pragma once
#include "scenes_manager/scenes_processor/actions/scene_actions.h"

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace ScenesManager {

class HubBackupRun final : public ControllerCoreActions {
public:
    HubBackupRun( ControllerFactory& controllerFactory,
                  Services::Service& hostService,
                  ScenesRequestHandlerInterface& scenesHandler );
    void run( const Action& action, ActionExecutorPtr actionExecutorPtr ) override;

protected:
    struct RequestData {
        std::string m_serviceName;
        MessageSubClasses m_requestType;
        JsonObject m_requestBody;
    };
    static RequestData getRequestData( const Action::Arguments& args );

private:
    ScenesRequestHandlerInterface& m_scenesHandler;
    Services::Service& m_hostService;
};

}  // namespace ScenesManager
}  // namespace HubPlatform