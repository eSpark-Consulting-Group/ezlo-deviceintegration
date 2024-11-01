#pragma once

#include "json_utils/json_fwd.h"

#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {

class Environment;
class Plugin;

class PluginsFactory final {
public:
    PluginsFactory( Environment& environment );
    std::unique_ptr< Plugin > createPlugin( const Json& joConfig );

private:
    Environment& m_environment;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
