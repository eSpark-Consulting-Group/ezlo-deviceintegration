#include "scenes_manager/scenes_processor/controller_interface/controller_factory.h"
#include "scenes_manager/scenes_processor/controller_interface/internal_controller.h"
#include "common_utils/hub_info_helper.h"

namespace HubPlatform {
namespace ScenesManager {

ControllerFactory::ControllerFactory( Services::Service& hostService,
                                      ExternalControllers::AbstractInterface& extControllersInterface )
: m_hostService( hostService )
, m_extControllersInterface( extControllersInterface ) {}

ControllerFactory::ControllerInterfacePtr ControllerFactory::getControllerInterface(
    const ControllerId& controllerId ) {
    if( controllerId == HubInfoHelper::getPkDevice() ) {
        return std::make_shared< InternalController >( m_hostService );
    }
    else {
        auto it = m_externalControllers.find( controllerId );
        if( it != m_externalControllers.end() ) {
            return it->second;
        }
        auto result = m_externalControllers.insert(
            { controllerId,
              std::make_shared< ExternalController >( controllerId, m_extControllersInterface, m_hostService ) } );
        return result.first->second;
    }
}

void ControllerFactory::itemAdded( const SceneEngine::ControllerInterface::Item& item ) {
    auto controllerId = json_get_str( item, "controllerId" );
    auto it = m_externalControllers.find( controllerId );
    if( it == m_externalControllers.end() ) {
        return;
    }
    it->second->itemAdded( item );
}

void ControllerFactory::deviceAdded( const SceneEngine::ControllerInterface::Device& device ) {
    auto controllerId = json_get_str( device, "controllerId" );
    auto it = m_externalControllers.find( controllerId );
    if( it == m_externalControllers.end() ) {
        return;
    }
    it->second->deviceAdded( device );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
