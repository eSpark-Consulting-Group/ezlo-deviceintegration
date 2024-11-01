#pragma once

#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "scenes_manager/scenes_processor/controller_interface/external_controller.h"
#include "external_controllers/remote_interface/ec_remote_interface.h"
#include "service_interfaces/service.h"

#include <memory>

namespace HubPlatform {
namespace ScenesManager {

class ControllerFactory : public SceneEngine::ControllerFactoryInterface {
public:
    using ControllerInterfacePtr = std::shared_ptr< SceneEngine::ControllerInterface >;
    using ExternalControllerPtr = std::shared_ptr< ExternalController >;
    using ExtControllers = std::map< ControllerId, ExternalControllerPtr >;

    ControllerFactory( Services::Service& hostService, ExternalControllers::AbstractInterface& );

    ControllerInterfacePtr getControllerInterface( const ControllerId& controllerId ) override;
    void itemAdded( const SceneEngine::ControllerInterface::Item& item ) override;
    void deviceAdded( const SceneEngine::ControllerInterface::Device& device ) override;

private:
    Services::Service& m_hostService;
    ExternalControllers::AbstractInterface& m_extControllersInterface;
    ExtControllers m_externalControllers;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
