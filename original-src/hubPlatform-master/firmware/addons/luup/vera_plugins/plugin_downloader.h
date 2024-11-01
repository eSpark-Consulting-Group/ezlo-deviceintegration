#pragma once
#include "json_utils/json_fwd.h"
#include <string>
#include <map>

// temporary
extern const std::string pluginsDbName;

class IDbClient;

class PluginDownloader final {
public:
    PluginDownloader( IDbClient& dbClient, std::string pluginId );

    /* Possible states of download progress:
     * "initial" - error state, main json was not correctly parsed
     * "uncompleted" - main json was parsed, but some files are missed
     * "downloaded" - all files were downloaded
     */

    using VeraDeviceId = int;                  // comes from vera project Device_Basic::m_iDevice_Num
    using VeraDeviceInternalId = std::string;  // comes from vera project Device_Basic::m_sID
    using DevicesInfo = std::map< VeraDeviceId, VeraDeviceInternalId >;

    bool fetchPluginInfo( const DevicesInfo& devicesInfo, bool force, const std::string& version = std::string() );

    bool downloadMissedFiles();

private:
    bool prepareFileList();
    bool readDevices();
    void saveFileInfo( const std::string& name, const std::string& absoluteName );

private:
    IDbClient& m_dbClient;
    JsonObject m_pluginDescription;
    const std::string m_pluginId;
};
