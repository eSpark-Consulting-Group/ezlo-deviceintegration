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
class GatewayConfig;

class GatewayPlugin final : public Plugin {
public:
    GatewayPlugin( Environment& environment,
                   std::unique_ptr< BaseConfig > baseConfigInfo,
                   std::unique_ptr< GatewayConfig > gatewayConfigInfo );

    void init() override;
    void startup() override;
    bool migrate() override;
    bool teardown( const Json& operationInfo ) override;
    bool uninstall() override;

private:
    std::unique_ptr< GatewayConfig > m_gatewayConfigInfo;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
