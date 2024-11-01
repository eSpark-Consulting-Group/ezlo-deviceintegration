#pragma once

#include "update_manager/remote_interface/um_remote_interface.h"

class IDbClient;

namespace HubPlatform {

namespace Services {
class Service;
}

namespace CoreDRemoteInterface {
class CoreDInterface;
}

namespace ScenesManager {
class BaseRemoteInterface;
}

namespace ExtensionsManager {

class AddonUtils;
class PluginUtils;
class Messenger;

class Environment {
public:
    virtual ~Environment() = default;

    virtual AddonUtils& getAddonUtils() = 0;
    virtual PluginUtils& getPluginUtils() = 0;
    virtual Messenger& getMessenger() = 0;
    virtual IDbClient& getDbClient() = 0;

    virtual const CoreDRemoteInterface::CoreDInterface& getCoreDInterface() const = 0;
    virtual CoreDRemoteInterface::CoreDInterface& getCoreDInterface() = 0;

    virtual const ScenesManager::BaseRemoteInterface& getScenesManagerInterface() const = 0;
    virtual ScenesManager::BaseRemoteInterface& getScenesManagerInterface() = 0;

    virtual bool isPluginUnavailable( const std::string& pluginId ) const = 0;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
