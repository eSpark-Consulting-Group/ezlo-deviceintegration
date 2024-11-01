#pragma once

#include "extensions_manager/remote_interface/em_remote_interface.h"

#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class RemoteInterfaceFactory {
protected:
    using RemoteInterfaceUPtr = std::unique_ptr< RemoteInterface >;

    RemoteInterfaceUPtr createRemoteInterface( Services::Service& hostService,
                                               RemoteInterface::PluginUtilsUPtr pluginUtils,
                                               RemoteInterface::AnonymousPluginUtilsUPtr anonymousPluginUtils ) const {
        return std::unique_ptr< RemoteInterface >(
            new RemoteInterface( hostService, std::move( pluginUtils ), std::move( anonymousPluginUtils ) ) );
    }
};

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
