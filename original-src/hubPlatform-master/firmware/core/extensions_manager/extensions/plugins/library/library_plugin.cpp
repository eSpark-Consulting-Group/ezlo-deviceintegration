#include "extensions_manager/extensions/plugins/library/library_plugin.h"
#include "extensions_manager/em_environment.h"
#include "extensions_manager/em_messenger.h"
#include "service_interfaces/service.h"

#include "plugin_utils/plugin_utils.h"
#include "log_utils/ezlo_log.h"
#include "jsonrpc/jsonrpc.h"

#include "database_client/dbqueryparser.h"
#include <cassert>

namespace HubPlatform {
namespace ExtensionsManager {

LibraryPlugin::LibraryPlugin( Environment& environment,
                              std::unique_ptr< BaseConfig > baseConfigInfo,
                              std::unique_ptr< LibraryConfig > libraryConfigInfo )
: Plugin( environment, std::move( baseConfigInfo ) )
, m_libraryConfigInfo( std::move( libraryConfigInfo ) ) {
    assert( m_libraryConfigInfo );
}

void LibraryPlugin::startup() {
    Plugin::startup();
    getEnvironment().getMessenger().sendAddLibrary( getBaseConfig().summary.id, m_libraryConfigInfo->m_entryPoint );
}

bool LibraryPlugin::uninstall() {
    getEnvironment().getMessenger().sendRemoveLibrary( getBaseConfig().summary.id );
    if( !Plugin::uninstall() ) {
        return false;
    }
    return true;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
