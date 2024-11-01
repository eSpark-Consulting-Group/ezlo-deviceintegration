#pragma once

#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "scenes_manager/scenes_processor/controller_interface/internal_controller.h"

namespace HubPlatform {
namespace Services {
class Service;
}  // namespace Services
}  // namespace HubPlatform

namespace SceneEngine {

class FakeControllerFactory : public ControllerFactoryInterface {
public:
    using MakeControllerFunction = std::function< ControllerInterfacePtr( const ControllerId& ) >;

    explicit FakeControllerFactory( MakeControllerFunction makeControllerFunction )
    : m_makeControllerFunction( std::move( makeControllerFunction ) ) {}

    ControllerInterfacePtr getControllerInterface( const ControllerId& controllerId ) override {
        return m_makeControllerFunction( controllerId );
    }

    void itemAdded( const ControllerInterface::Item& item ) override {}
    void deviceAdded( const ControllerInterface::Device& device ) override {}

private:
    MakeControllerFunction m_makeControllerFunction;
};

}  // namespace SceneEngine
