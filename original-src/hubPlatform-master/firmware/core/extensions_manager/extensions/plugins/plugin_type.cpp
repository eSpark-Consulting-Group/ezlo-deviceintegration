#include "extensions_manager/extensions/plugins/plugin_type.h"

#include <map>

namespace HubPlatform {
namespace ExtensionsManager {

PluginType stringToPluginType( const std::string& pluginTypeString ) {
    auto type = PluginType::NONE;

    static const std::map< std::string, PluginType > pluginTypes = {
        { "gateway", PluginType::GATEWAY },
        { "library", PluginType::LIBRARY },
        { "functional_unit", PluginType::FUNCTIONAL_UNIT },
        { "extension", PluginType::EXTENTION },
    };

    auto it = pluginTypes.find( pluginTypeString );
    if( it != pluginTypes.end() ) {
        type = it->second;
    }

    return type;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
