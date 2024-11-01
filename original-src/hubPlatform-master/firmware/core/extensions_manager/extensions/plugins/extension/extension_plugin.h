#pragma once

#include "extensions_manager/extensions/plugins/plugin.h"

#include <memory>

namespace HubPlatform {

namespace CoreDRemoteInterface {
class CoreDRemoteInterface;
}

namespace ExtensionsManager {

class Environment;
class BaseConfig;

class ExtensionPlugin final : public Plugin {
public:
    ExtensionPlugin( Environment& environment, std::unique_ptr< BaseConfig > baseConfigInfo );

    void init() override;
    bool uninstall() override;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
