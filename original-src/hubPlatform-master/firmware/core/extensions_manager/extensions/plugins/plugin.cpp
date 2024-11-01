#include "extensions_manager/extensions/plugins/plugin.h"
#include "extensions_manager/resources/em_resources.h"
#include "extensions_manager/em_messenger.h"
#include "extensions_manager/em_environment.h"

#include "scenes_manager/remote_interface/scenes_remote_interface.h"

#include "plugin_utils/plugin_utils.h"
#include "log_utils/ezlo_log.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

#include <tiny/tiny_consolecolors.h>

#include <cassert>

namespace HubPlatform {
namespace ExtensionsManager {

Plugin::Plugin( Environment& environment, std::unique_ptr< BaseConfig > baseConfig )
: m_environment( environment )
, m_baseConfig( std::move( baseConfig ) )
, m_status( Status::Stopped ) {
    assert( m_baseConfig );
}

Plugin::~Plugin() {}

const BaseConfig& Plugin::getBaseConfig() const {
    return *m_baseConfig;
}

void Plugin::setStatus( Status status ) {
    m_status = status;
}

Plugin::Status Plugin::getStatus() const {
    return m_status;
}

void Plugin::init() {
    if( m_status == Status::Started ) {
        Log::info( cc::info( "Plugin restarted: " ) + cc::note( getBaseConfig().summary.toString() ) );
    }
    else {
        Log::info( cc::info( "Plugin started: " ) + cc::note( getBaseConfig().summary.toString() ) );
    }

    m_status = Status::Started;

    auto pluginJson = JSON(
        db::ID_FIELD_NAME << getBaseConfig().summary.id << "version" << getBaseConfig().summary.version.toString()
                          << Resources::PluginRecord::Permissions << getBaseConfig().permissions
                          << Resources::PluginRecord::ExecutionPolicy << toString( getBaseConfig().executionPolicy ) );

    if( getBaseConfig().extensions ) {
        pluginJson[ Resources::PluginRecord::Extensions ] = getBaseConfig().extensions->toJson();
    }

    getEnvironment().getDbClient().update( Resources::DBTableNames::Plugins, JSON_EMPTY_OBJ, pluginJson, true, false );

    getEnvironment().getMessenger().sendPluginRestarted( getBaseConfig().summary.id );

    if( !getBaseConfig().sceneBlocksJson.empty() ) {
        auto sceneBlocksPath = getEnvironment().getPluginUtils().constructScriptName( getBaseConfig().summary.id,
                                                                                      getBaseConfig().sceneBlocksJson );

        Log::info( cc::info( "Set scene block template: " ) + cc::note( sceneBlocksPath ) );

        const int triesNumber = 3;
        for( int i = 0; i < triesNumber; ++i ) {
            if( getEnvironment().getScenesManagerInterface().setSceneBlocksSource( sceneBlocksPath ) ) {
                break;
            }
        }
    }
}

void Plugin::startup() {
    auto startupScript = getEnvironment().getPluginUtils().constructScriptName( getBaseConfig().summary.id,
                                                                                getBaseConfig().startupScript );

    Log::info( cc::info( "Plugin: request to start the plugin (" ) + cc::note( getBaseConfig().summary.toString() ) +
               cc::info( ", " ) + cc::note( startupScript ) + cc::info( ")" ) );

    if( !getBaseConfig().startupScript.empty() )
        getEnvironment().getMessenger().sendExecutePluginScript( startupScript );
}

bool Plugin::migrate() {
    bool ok = true;
    if( !getBaseConfig().initializeMigrationScript.empty() ) {
        const auto initializeMigrationScriptName = constructScriptName( getBaseConfig().initializeMigrationScript );

        Log::info( cc::info( "Plugin: request to initialize migration(" ) +
                   cc::note( getBaseConfig().summary.toString() ) + cc::info( ", " ) +
                   cc::note( initializeMigrationScriptName ) + cc::info( ")" ) );

        const auto scriptResult =
            getEnvironment().getMessenger().sendExecutePluginScriptSync( initializeMigrationScriptName );
        if( scriptResult.hasError() ) {
            Log::error( cc::info( "Initialize migration failed: " ) + cc::error( scriptResult.getError() ) );
            ok = false;
        }
    }

    if( ok && !getBaseConfig().migrationScript.empty() ) {
        const auto migrateDeviceScriptName = constructScriptName( getBaseConfig().migrationScript );

        Log::info( cc::info( "Plugin: request to migrate device(" ) + cc::note( getBaseConfig().summary.toString() ) +
                   cc::info( ", " ) + cc::note( migrateDeviceScriptName ) + cc::info( ")" ) );

        bool needContinueMigrate = false;
        do {
            const auto scriptResult =
                getEnvironment().getMessenger().sendExecutePluginScriptSync( migrateDeviceScriptName );

            if( scriptResult.hasError() ) {
                Log::error( cc::info( "Migration failed: " ) + cc::error( scriptResult.getError() ) );
                ok = false;
                break;
            }

            needContinueMigrate = scriptResult.getValue( 0 );
        } while( needContinueMigrate );
    }

    if( !getBaseConfig().finalizeMigrationScript.empty() ) {
        const auto finalizeMigrationScriptName = constructScriptName( getBaseConfig().finalizeMigrationScript );

        Log::info( cc::info( "Plugin: request to finalize migration(" ) +
                   cc::note( getBaseConfig().summary.toString() ) + cc::info( ", " ) +
                   cc::note( finalizeMigrationScriptName ) + cc::info( ")" ) );

        const auto scriptResult =
            getEnvironment().getMessenger().sendExecutePluginScriptSync( finalizeMigrationScriptName );

        if( scriptResult.hasError() ) {
            Log::error( cc::info( "Finalize migration failed : " ) + cc::error( scriptResult.getError() ) );
            ok = false;
        }
    }
    return ok;
}

bool Plugin::teardown( const Json& operationInfo ) {
    Log::info( cc::info( "Tearing down plugin: " ) + cc::note( getBaseConfig().summary.toString() ) );
    if( m_status == Status::Stopped ) {
        Log::warn( cc::warn( "Plugin is already stopped." ) );
        return false;
    }
    if( !getBaseConfig().teardownScript.empty() )
        getEnvironment().getMessenger().sendExecutePluginScript(
            getEnvironment().getPluginUtils().constructScriptName( getBaseConfig().summary.id,
                                                                   getBaseConfig().teardownScript ),
            operationInfo );

    m_status = Status::Stopped;

    return true;
}

bool Plugin::uninstall() {
    if( !getBaseConfig().sceneBlocksJson.empty() ) {
        getEnvironment().getScenesManagerInterface().removeSceneBlocksSource( getBaseConfig().sceneBlocksJson );
    }

    getEnvironment().getDbClient().remove( Resources::DBTableNames::Plugins,
                                           JSON( db::ID_FIELD_NAME << getBaseConfig().summary.id ) );
    return true;
}

bool Plugin::hasExtensionFor( const std::string& pluginId, const std::string& extensionName ) const {
    const auto& baseConfig = getBaseConfig();
    if( baseConfig.extensions ) {
        const auto& extensions = *baseConfig.extensions;
        for( const auto& extension : extensions.pluginExtensions ) {
            if( extension.pluginId == pluginId ) {
                if( !extensionName.empty() ) {
                    for( const auto& extensionConfig : extension.configs ) {
                        if( extensionConfig.name == extensionName ) {
                            return true;
                        }
                    }
                }
                else {
                    return true;
                }
            }
        }
    }

    return false;
}

const Environment& Plugin::getEnvironment() const {
    return m_environment;
}

Environment& Plugin::getEnvironment() {
    return m_environment;
}

std::string Plugin::constructScriptName( const std::string& script ) {
    return getEnvironment().getPluginUtils().constructScriptName( getBaseConfig().summary.id, script );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
