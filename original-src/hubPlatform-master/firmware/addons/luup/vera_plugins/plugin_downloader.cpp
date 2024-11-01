
#include "luup/vera_plugins/plugin_downloader.h"

#include "luup/vera_plugins/file_utils.h"
#include "common_utils/curl_wrapper.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include <fstream>
#include <cassert>
#include "database_client/idbclient.h"

namespace {

// temporary
const std::string pkDevice = "55100234";
const std::string hwKey = "qJxvLZha4HKE7oZuqCoHcFlSDa1WGCOd";

// @todo: move it somewhere to global config
const std::string downloadServerMain = "apps.mios.com";
const std::string downloadServerBackup = "apps.mios.com";
const std::string pluginDownloadPath = "./plugin_download/";
const std::string pluginInstallPath = "./plugin_install/";

}  // namespace

PluginDownloader::PluginDownloader( IDbClient& dbClient, std::string pluginId )
: m_dbClient( dbClient )
, m_pluginId( std::move( pluginId ) ) {}

bool PluginDownloader::fetchPluginInfo( const DevicesInfo& devicesInfo, bool force, const std::string& version ) {
    // Check if plugin with this id already exists.
    JsonObject existingPlugin = m_dbClient.findOne( pluginsDbName, JSON( "pluginId" << m_pluginId ), nullptr );

    if( !existingPlugin.empty() ) {
        if( force || existingPlugin[ "downloadState" ] == "initial" ) {
            m_dbClient.remove( pluginsDbName, JSON( "pluginId" << m_pluginId ) );
        }
        else if( existingPlugin[ "downloadState" ] == "downloaded" ) {
            Log::info( "Downloading: plugin with id " + m_pluginId + " is already downloaded" );
            return true;
        }
        else {
            // State is "uncompleted", so info was already fetched
            return true;
        }
    }

    std::string path = "/get_plugin_json2.php?plugin=" + m_pluginId;
    if( !version.empty() )
        path += "&version=" + version;
    path += "&ap=" + pkDevice + "&HW_Key=" + hwKey;

    for( const auto& it : devicesInfo )
        path += "&D_" + std::to_string( it.first ) + "=" + it.second;

    std::string url = "https://" + downloadServerMain + path;
    std::string result;
    std::stringstream jsonFileStream;

    if( !CurlWrapper::downloadFile( url, jsonFileStream, result ) ) {
        Log::warn( "Downloading: trying backup server for plugin " + m_pluginId + ", result: " + result );
        url = "https://" + downloadServerBackup + path;
        if( !CurlWrapper::downloadFile( url, jsonFileStream, result ) ) {
            Log::error( "Downloading: could not get plugin description file for plugin " + m_pluginId +
                        ", result: " + result );
            return false;
        }
    }

    m_pluginDescription = Json::parse( jsonFileStream.str() );

    if( existingPlugin.empty() ) {
        // Write initial info about plugin.
        m_dbClient.insert( pluginsDbName,
                           JSON( "pluginId" << m_pluginId << "downloadState"
                                            << "initial"
                                            << "title" << m_pluginDescription[ "Title" ] << "version"
                                            << m_pluginDescription[ "Version" ] << "downloadedFiles" << JSON_EMPTY_ARR
                                            << "missedFiles" << JSON_EMPTY_ARR << "devices" << JSON_EMPTY_ARR ) );
    }

    if( !prepareFileList() || !readDevices() ) {
        return false;
    }

    return true;
}

bool PluginDownloader::prepareFileList() {
    auto versionIt = m_pluginDescription.find( "Version" );
    if( versionIt == m_pluginDescription.end() && !versionIt.value().is_null() ) {
        Log::error( "Downloading: could not get version of plugin " + m_pluginId );
        return false;
    }
    const std::string& version = versionIt.value();

    auto filesIt = m_pluginDescription.find( "Files" );
    if( filesIt == m_pluginDescription.end() && !filesIt.value().is_array() ) {
        Log::error( "Downloading: could not get file list of plugin " + m_pluginId );
        return false;
    }

    JsonArray missedFiles = JSON_EMPTY_ARR;

    const JsonArray& files = filesIt.value();
    for( auto& file : files ) {
        std::string sourceName;
        auto sourceNameIt = file.find( "SourceName" );
        if( sourceNameIt == file.end() || sourceNameIt.value().is_null() ) {
            Log::warn( "Downloading: no source name for file in plugin " + m_pluginId );
            continue;
        }
        sourceName = sourceNameIt->get< std::string >();

        std::string sourcePath;
        auto sourcePathIt = file.find( "SourcePath" );
        if( sourcePathIt != file.end() && !sourcePathIt.value().is_null() )
            sourcePath = sourcePathIt->get< std::string >();

        std::string destName;
        auto destNameIt = file.find( "DestName" );
        if( destNameIt == file.end() || destNameIt.value().is_null() )
            destName = sourceName;
        else
            destName = destNameIt->get< std::string >();

        std::string destPath;
        auto destPathIt = file.find( "DestPath" );
        if( destPathIt != file.end() && !destPathIt.value().is_null() )
            destPath = destPathIt->get< std::string >();

        bool compressed = false;
        auto compressedIt = file.find( "Compress" );
        if( compressedIt != file.end() && !compressedIt.value().is_null() )
            compressed = compressedIt->get< std::string >() == "1";

        bool encrypted = false;
        auto encryptedIt = file.find( "Encrypt" );
        if( encryptedIt != file.end() && !encryptedIt.value().is_null() )
            encrypted = encryptedIt->get< std::string >() == "1";

        std::string targetFileName = ( destPath.empty() ? "" : destPath + "/" ) + destName;
        if( compressed )
            targetFileName += ".lzo";
        if( encrypted )
            targetFileName += "enc";

        std::string fileUrlPath = "/plugins/" + m_pluginId + "/" + version + "/" +
                                  ( sourcePath.empty() ? "" : sourcePath + "/" ) + sourceName;
        if( compressed )
            fileUrlPath += ".lzo";
        if( encrypted )
            fileUrlPath += "enc";

        targetFileName = pluginDownloadPath + targetFileName;
        missedFiles.push_back(
            JSON( "name" << destName << "absoluteName" << targetFileName << "urlPath" << fileUrlPath ) );
    }

    m_dbClient.update( pluginsDbName,
                       JSON( "pluginId" << m_pluginId ),
                       JSON( "$set" << JSON( "missedFiles" << missedFiles << "downloadState"
                                                           << "uncompleted" ) ),
                       false,
                       true );

    return true;
}

bool PluginDownloader::downloadMissedFiles() {
    bool allDownloaded = true;
    const JsonObject fieldsToReturn = JSON( "missedFiles" << true );
    JsonObject missedFiles = m_dbClient.findOne( pluginsDbName, JSON( "pluginId" << m_pluginId ), &fieldsToReturn );

    for( const auto& file : missedFiles[ "missedFiles" ] ) {
        const std::string fileUrlPath = json_get_str( file, "urlPath" );
        const std::string targetFileName = json_get_str( file, "absoluteName" );

        {
            const std::string dir = FileUtils::getDir( targetFileName );
            if( !FileUtils::dirExists( dir ) ) {
                FileUtils::makeDir( dir );
            }
        }

        std::ofstream targetFile( targetFileName );
        if( targetFile.is_open() ) {
            std::string fileUrl = "http://" + downloadServerMain + fileUrlPath;
            std::string result;
            bool success = CurlWrapper::downloadFile( fileUrl, targetFile, result );
            if( !success ) {
                Log::warn( "Downloading: could not download file " + fileUrl + " for plugin " + m_pluginId +
                           ", result: " + result + ". Trying backup server..." );
                fileUrl = "http://" + downloadServerBackup + fileUrlPath;
                success = CurlWrapper::downloadFile( fileUrl, targetFile, result );
                if( !success ) {
                    Log::error( "Downloading: could not download file " + fileUrl + " for plugin " + m_pluginId +
                                ", result: " + result );

                    allDownloaded = false;
                    // Delete file.
                    if( remove( targetFileName.c_str() ) != 0 ) {
                        Log::error( "Could not delete file " + targetFileName );
                    }
                }
            }
            if( success ) {
                // Save each file's info to Db.
                saveFileInfo( json_get_str( file, "name" ), targetFileName );
            }
        }
        else
            Log::error( "Downloading: Could not open file for write " + targetFileName );
    }

    if( allDownloaded ) {
        // Set state to downloaded.
        m_dbClient.update( pluginsDbName,
                           JSON( "pluginId" << m_pluginId ),
                           JSON( "$set" << JSON( "downloadState"
                                                 << "downloaded" ) ),
                           false,
                           true );
    }

    return allDownloaded;
}

void PluginDownloader::saveFileInfo( const std::string& name, const std::string& absoluteName ) {
    // Move file info from missedFiles array to downloadedFiles array.
    const JsonObject fieldsToReturn = JSON( "downloadedFiles" << true << "missedFiles" << true );
    JsonObject files = m_dbClient.findOne( pluginsDbName, JSON( "pluginId" << m_pluginId ), &fieldsToReturn );

    JsonArray& downloadedFilesArray = files[ "downloadedFiles" ];
    JsonArray& missedFilesArray = files[ "missedFiles" ];
    assert( downloadedFilesArray.is_array() && missedFilesArray.is_array() );

    const JsonObject newFile = JSON( "name" << name << "absoluteName" << absoluteName );
    downloadedFilesArray.push_back( newFile );
    auto it = std::find_if( missedFilesArray.begin(), missedFilesArray.end(), [&newFile]( const JsonObject& item ) {
        return json_get_str( item, "name" ) == json_get_str( newFile, "name" ) &&
               json_get_str( item, "absoluteName" ) == json_get_str( newFile, "absoluteName" );
    } );
    if( it != missedFilesArray.end() ) {
        missedFilesArray.erase( it );
    }

    m_dbClient.update(
        pluginsDbName,
        JSON( "pluginId" << m_pluginId ),
        JSON( "$set" << JSON( "downloadedFiles" << downloadedFilesArray << "missedFiles" << missedFilesArray ) ),
        false,
        true );
}

bool PluginDownloader::readDevices() {
    auto devicesIt = m_pluginDescription.find( "Devices" );
    if( devicesIt == m_pluginDescription.end() && !devicesIt.value().is_array() ) {
        Log::error( "Downloading: could not get device list of plugin " + m_pluginId );
        return false;
    }

    JsonArray newDevices = JSON_EMPTY_ARR;
    const JsonArray& devices = devicesIt.value();
    for( auto& device : devices ) {
        // @todo: find out which of these fields are mandatory

        const std::string deviceFileName = json_get_str( device, "DeviceFileName" );
        if( deviceFileName.empty() ) {
            Log::warn( "Downloading: could not get device file name of plugin " + m_pluginId );
        }

        const std::string deviceType = json_get_str( device, "DeviceType" );
        if( deviceType.empty() ) {
            Log::warn( "Downloading: could not determine device type of plugin " + m_pluginId );
        }

        const std::string implFileName = json_get_str( device, "ImplFile" );
        if( implFileName.empty() ) {
            Log::warn( "Downloading: could not get impl file name of plugin " + m_pluginId );
        }

        const std::string invisible = json_get_str( device, "Invisible" );
        if( invisible.empty() ) {
            Log::warn( "Downloading: could not get device visibility value of plugin " + m_pluginId );
        }

        newDevices.push_back( JSON( "deviceFileName" << deviceFileName << "deviceType" << deviceType << "implFile"
                                                     << implFileName << "invisible" << ( invisible == "1" ) ) );
    }

    // Write to Db info about devices.
    m_dbClient.update( pluginsDbName,
                       JSON( "pluginId" << m_pluginId ),
                       JSON( "$set" << JSON( "devices" << newDevices ) ),
                       false,
                       true );

    return true;
}
