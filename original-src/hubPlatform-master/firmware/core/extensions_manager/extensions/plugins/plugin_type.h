#pragma once

#include <string>

namespace HubPlatform {
namespace ExtensionsManager {

enum class PluginType { NONE = 0, GATEWAY, LIBRARY, FUNCTIONAL_UNIT, EXTENTION };

PluginType stringToPluginType( const std::string& pluginTypeString );

}  // namespace ExtensionsManager
}  // namespace HubPlatform
