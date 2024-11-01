#pragma once

#include "action_interface.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_processor/scene_request_handler_interface.h"
#include <memory>

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace ScenesManager {

namespace SceneActions {
using ActionInterfacePtr = std::shared_ptr< ActionInterface >;

ActionInterfacePtr getActionForMethod( const std::string& method_name,
                                       Services::Service& hostService,
                                       SceneEngine::ControllerFactoryInterface& controllerFactory,
                                       SceneEngine::Expressions::ExpressionManager& expressionManager,
                                       ScenesRequestHandlerInterface& scenesHandler );
}  // namespace SceneActions

}  // namespace ScenesManager
}  // namespace HubPlatform