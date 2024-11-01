
#include "luup/vera_plugins/plugins_manager.h"

#include "database_client/idbclient.h"
#include "luup/vera_plugins/plugin_downloader.h"
#include "luup/vera_plugins/plugin_installer.h"
#include "luup/vera_plugins/file_utils.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

const std::string pluginsDbName = "db.addons.luup.plugins";

PluginsManager::PluginsManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool PluginsManager::installPlugin( const std::string& pluginId,
                                    const std::string& version,
                                    const std::string& aesKey,
                                    const std::string& stateVariables ) {
    try {
        if( !aesKey.empty() ) {
            // @todo: find out what it is for, in Vera it is saved as "key" field in json
            // that is created when devices for plugin are creating
        }

        if( !stateVariables.empty() ) {
            // @todo: list of variables that are passed to Device_Basic::SetVariables() in Vera
        }

        PluginDownloader downloader( m_dbClient, pluginId );
        if( !downloader.fetchPluginInfo( {}, false, version ) || !downloader.downloadMissedFiles() ) {
            return false;
        }

        PluginInstaller installer( m_dbClient, pluginId );
        if( !installer.createDevices() || !installer.createServices()
            /*...*/ ) {
            return false;
        }

        return true;
    }
    catch( std::exception& ex ) {
        Log::error( ex.what() );
        return false;
    }
}

bool PluginsManager::removePlugin( const std::string& /*pluginId*/ ) {
    return true;
}
