
#include "luup/vera_plugins/plugin_installer.h"

#include "database_client/idbclient.h"
#include "luup/vera_plugins/file_utils.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

PluginInstaller::PluginInstaller( IDbClient& dbClient, std::string pluginId )
: m_dbClient( dbClient )
, m_pluginId( std::move( pluginId ) ) {}

bool PluginInstaller::createDevices() {
    // Get from Db list of devices for this plugin and parse files.
    const JsonObject fieldsToReturn = JSON( "devices" << true );
    JsonObject result = m_dbClient.findOne( pluginsDbName, JSON( "pluginId" << m_pluginId ), &fieldsToReturn );
    JsonArray& devices = result[ "devices" ];
    if( !devices.is_array() || devices.empty() ) {
        return false;
    }

    for( const auto& deviceJsonObject : devices ) {
        const std::string deviceFileName = json_get_str( deviceJsonObject, "DeviceFileName" );
        // @todo: map file name to target file name on hub (from downloadedFiles)
        XmlDocument xml = FileUtils::parseXmlFile( deviceFileName );
        if( !xml.isValid() ) {
            Log::error( "Could not parse device file for plugin " + m_pluginId );
            return false;
        }

        XmlNode root = xml.getRootNode();
        XmlNode deviceNode = root.tryGetChildNode( "device" );
        XmlNode friendlyNameNode = deviceNode.getChildNode( "friendlyName" );
        if( friendlyNameNode ) {
            std::string friendlyName;
            if( friendlyNameNode.getTextFromNode( friendlyName ) ) {
                // @todo: save the name
            }
        }
    }

    return true;
}

bool PluginInstaller::createServices() {
    return true;
}
