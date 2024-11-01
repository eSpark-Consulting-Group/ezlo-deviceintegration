#pragma once

#include "scenes_manager/scenes_engine/controller_interface/controller_interface.h"
#include <memory>

namespace SceneEngine {

class ControllerFactoryInterface {
public:
    using ControllerInterfacePtr = std::shared_ptr< ControllerInterface >;
    using ControllerId = ControllerInterface::ControllerId;

    virtual ~ControllerFactoryInterface() = default;

    virtual ControllerInterfacePtr getControllerInterface( const ControllerId& controllerId ) = 0;
    virtual void itemAdded( const ControllerInterface::Item& item ) = 0;
    virtual void deviceAdded( const ControllerInterface::Device& device ) = 0;
};

}  // namespace SceneEngine
