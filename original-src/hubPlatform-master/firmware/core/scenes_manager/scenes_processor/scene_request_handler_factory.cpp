#include "scenes_manager/scenes_processor/scene_request_handler_factory.h"
#include "scenes_manager/scenes_processor/scene_request_handler.h"
#include "common_utils/cpp14.h"

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
    ScenesRequestHandlerInterface::AddServiceTask addServiceTask ) {
    //
    return std::make_unique< ScenesRequestHandler >( client,
                                                     controllerId,
                                                     sceneManager,
                                                     clock,
                                                     coreInterface,
                                                     extensionsManagerInterface,
                                                     expressions,
                                                     deviceItemGroupManager,
                                                     sendBroadcast,
                                                     addServiceTask );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
