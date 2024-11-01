#include "extensions_manager/extensions/plugins/extension/extension_plugin.h"
#include "extensions_manager/em_environment.h"

#include "plugin_utils/plugin_utils.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

#include <cassert>

namespace HubPlatform {
namespace ExtensionsManager {

ExtensionPlugin::ExtensionPlugin( Environment& environment, std::unique_ptr< BaseConfig > baseConfigInfo )
: Plugin( environment, std::move( baseConfigInfo ) ) {}

void ExtensionPlugin::init() {
    Plugin::init();
}

bool ExtensionPlugin::uninstall() {
    if( !Plugin::uninstall() ) {
        return false;
    }

    return true;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
