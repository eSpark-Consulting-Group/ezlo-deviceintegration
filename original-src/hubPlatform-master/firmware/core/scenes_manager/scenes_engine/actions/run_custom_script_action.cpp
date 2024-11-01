// Extracted from action.h/cpp
#include "scenes_manager/scenes_engine/actions/run_custom_script_action.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "database_client/dbqueryparser.h"

namespace SceneEngine {

RunCustomScriptAction::RunCustomScriptAction( HubPlatform::ExtensionsManager::RemoteInterface& extensionManager,
                                              InitData&& initData )
: Action( std::move( initData ) )
, m_extensionManager( extensionManager ) {
    auto customPluginId = json_get_str( getArgs(), "script" );
    int tries, triesLimit = 3;

    // TODO: EFWR-3455: this part can be moved outside. Throw an exception here in case of an error.
    // Outside, then, try to re-acquire the plugin's data in loop, and, if Ok after some time, add a scene to running
    // scenes registry.
    for( tries = 0; !m_remoteAnonymousPlugin && tries < triesLimit; tries++ ) {
        LOGGER_TRACE( cc::trace( "acquire plugin, try " ) + cc::num10( tries ) );
        m_remoteAnonymousPlugin = m_extensionManager.acquireAnonymousPlugin( customPluginId );
        if( m_remoteAnonymousPlugin.get() ) {
            break;
        }
        else {
            auto pluginsList = m_extensionManager.listAnonymousPlugins();
            auto findByPluginId = [&customPluginId]( const HubPlatform::ExtensionsManager::AnonymousPlugin& plugin ) {
                return json_get_str( plugin.toJson(), db::ID_FIELD_NAME ) == customPluginId;
            };

            auto foundIf = std::find_if( begin( pluginsList ), end( pluginsList ), findByPluginId );
            if( foundIf != end( pluginsList ) ) {
                LOGGER_TRACE( cc::trace( "continue, a plugin with the id exists: " ) + customPluginId );
                std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
                continue;
            }
        }
    }
    if( m_remoteAnonymousPlugin ) {
        LOGGER_DEBUG( cc::debug( "A custom script has been acquired: " ) + cc::note( customPluginId ) );
    }
    else {
        LOGGER_ERROR( cc::error( "failed to acquire a custom script: " ) + cc::note( customPluginId ) );
        throw SceneEngine::SceneException( Errors::ANONYMOUS_PLUGIN_FAILED_TO_ACQUIRE );
    }
}

RunPluginScriptAction::RunPluginScriptAction( HubPlatform::ExtensionsManager::RemoteInterface& extensionManager,
                                              InitData&& initData )
: Action( std::move( initData ) )
, m_extensionManager( extensionManager ) {
    LOGGER_TRACE( cc::trace( "RunPluginScriptAction constructor, no checks of plugin existence here" ) );
}

}  // namespace SceneEngine
