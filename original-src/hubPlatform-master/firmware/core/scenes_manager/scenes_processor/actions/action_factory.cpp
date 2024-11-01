#include "action_factory.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "group_toggle_value.h"
#include "hub_backup_action.h"
#include "forward_notifications.h"

namespace HubPlatform {
namespace ScenesManager {
namespace SceneActions {

ActionInterfacePtr getActionForMethod( const std::string& method_name,
                                       Services::Service& hostService,
                                       SceneEngine::ControllerFactoryInterface& controllerFactory,
                                       SceneEngine::Expressions::ExpressionManager& expressionManager,
                                       ScenesRequestHandlerInterface& scenesHandler ) {
    using namespace ActionFields;
    static const std::map< std::string, std::shared_ptr< ActionInterface > > actionByName = {
        { ActionNames::SetItemValue, std::make_shared< SetItemValue >( controllerFactory ) },
        { ActionNames::GroupSetItemValue, std::make_shared< GroupSetItemValue >( controllerFactory, hostService ) },
        { ActionNames::SetDeviceArmed, std::make_shared< DeviceArmed >( controllerFactory ) },
        { ActionNames::GroupSetDeviceArmed, std::make_shared< GroupDeviceArmed >( controllerFactory, hostService ) },
        { ActionNames::GroupToggleValue, std::make_shared< GroupToggleValueAction >( controllerFactory, hostService ) },
        { ActionNames::SendCloudAbstractCommand, std::make_shared< SendCloudAbstractCommand >( hostService ) },
        { ActionNames::SwitchHouseMode, std::make_shared< SwitchHouseMode >( controllerFactory ) },
        { ActionNames::SendHttpRequest, std::make_shared< SendHttpRequest >( hostService ) },
        { ActionNames::RunCustomScript, std::make_shared< RunLuaCodeAction >( hostService ) },
        { ActionNames::RunPluginScript, std::make_shared< RunLuaPluginScriptAction >( hostService ) },
        { ActionNames::RunScene, std::make_shared< RunSceneAction >( hostService ) },
        { ActionNames::StopScene, std::make_shared< StopSceneAction >( hostService ) },
        { ActionNames::SetSceneState, std::make_shared< SetSceneStateAction >( hostService ) },
        { ActionNames::RebootHub, std::make_shared< RebootHub >( hostService ) },
        { ActionNames::CloudAPI, std::make_shared< CloudAPICommand >( hostService ) },
        { ActionNames::ResetHub, std::make_shared< ResetHubAction >( hostService ) },
        { ActionNames::ResetLatch, std::make_shared< ResetLatchAction >( hostService ) },
        { ActionNames::SetVariable, std::make_shared< SetVariableAction >( hostService ) },
        { ActionNames::ResetSceneLatches, std::make_shared< ResetSceneLatchesAction >( hostService ) },
        { ActionNames::SetExpression, std::make_shared< SetExpressionAction >( hostService ) },
        { ActionNames::ToggleValue,
          std::make_shared< ToggleValueAction >( controllerFactory, expressionManager, scenesHandler ) },
        { ActionNames::HubBackupAction,
          std::make_shared< HubBackupRun >( controllerFactory, hostService, scenesHandler ) },
        { ActionNames::ForwardNotifications, std::make_shared< ForwardNotifications >( hostService ) },
    };

    auto it = actionByName.find( method_name );
    if( it != actionByName.end() ) {
        return it->second;
    }
    return std::make_shared< UnknownAction >();
}

}  // namespace SceneActions
}  // namespace ScenesManager
}  // namespace HubPlatform