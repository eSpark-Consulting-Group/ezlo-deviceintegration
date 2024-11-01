#pragma once

#include "scenes_manager/scenes_processor/scene_request_handler_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "database_client/idbclient.h"
#include "cored/group_operations/device_item_group_manager.h"

namespace SceneEngine {
class IClock;
class SceneManager;

namespace Expressions {
class ExpressionManager;
}

}  // namespace SceneEngine

namespace ExtensionsManager {
class RemoteInterface;
}

namespace HubPlatform {
namespace ScenesManager {

std::unique_ptr< ScenesRequestHandlerInterface > makeScenesRequestHandler(
    IDbClient& client,
    const std::string& controllerId,
    SceneEngine::SceneManager& sceneManager,
    SceneEngine::IClock& clock,
    CoreDRemoteInterface::CoreDInterface& coreInterface,
    ExtensionsManager::RemoteInterface& extensionsManagerInterface,
    SceneEngine::Expressions::ExpressionManager& expressions,
    CoreD::DeviceItemGroupManager& deviceItemGroupManager,
    ScenesRequestHandlerInterface::BroadcastCallback sendBroadcast,
    ScenesRequestHandlerInterface::AddServiceTask addServiceTask );

}  // namespace ScenesManager
}  // namespace HubPlatform
