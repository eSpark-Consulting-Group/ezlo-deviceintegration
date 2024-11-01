#pragma once
#include "action_interface.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "scenes_manager/scenes_processor/scene_request_handler_interface.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "scene_actions.h"
#include "retry_command_manager/retry_command_manager.h"
#include "retry_command_manager/cloud_command.h"

#include "jsonrpc/message.h"

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace ScenesManager {

class ResponseMessage : public rpc::Message {
public:
    GenericSenderPtr getNetworkObject() const override {
        return pClient_.get();
    }

private:
    using ClientType = tiny::ws::client::client;
    using ClientTypePtr = tiny::retain_release_ptr< ClientType, rpc::LegacySynchronizationTraits< ClientType > >;
    mutable ClientTypePtr pClient_;
};

class ForwardNotifications : public ActionInterface {
public:
    ForwardNotifications( Services::Service& hostService );
    void run( const Action& action, ActionExecutorPtr actionExecutorPtr ) override;

private:
    using SendFinishCallbackPtr = std::shared_ptr< ActionInterface::SendFinishCallback >;
    JsonObject createControllerNotificationSendParams( const Action& action, SendFinishCallbackPtr callback );
    bool isStatusComplete( const JsonObject& message );
    JsonObject getDeviceInfo( const std::string& device );
    JsonArray getItems( const std::string& device );
    JsonObject getParentDevice( const JsonObject& device );
    JsonObject getScene( const std::string& device );
    JsonArray getTargets( const std::string& device );
    std::string getRoomName( const JsonObject& device );
    std::string temperatureScale();

private:
    Services::Service& m_hostService;
    std::unique_ptr< CoreDRemoteInterface::CoreDInterface > m_core;
    std::unique_ptr< RetryCommandManager > m_commandManager;
    std::shared_ptr< CloudCommand > m_command;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
