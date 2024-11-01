#include "common_utils/backup/backup_helper.h"
#include "common_utils/hub_state.h"

#include "system_utils/filesystem.h"

#include "jsonrpc/jsonrpc.h"
#include "tiny/tiny_process.h"

#include <fstream>

namespace HubPlatform {
namespace CommonUtils {

// request = {
//      "directory": "<intermediate backup root directory>",
//      "config": {
//          "version": "0.1",
//          "hub_type": "h2_secure.1",
//          "serial": "92000123",
//          "type": "native",
//          "software": { <result of hub.software.info.get> }
//      }
// }

BackupHelper::BackupHelper( IDbClient& dbClient, const JsonObject& request )
: m_dbClient( dbClient )
, m_backupDirectory( json_get_str( request, "directory" ) )
, m_crypt( json_get_str( request[ "config" ], "version" ) ) {
    LOGGER_INFO_RAW( cc::info( "Starting backup" ) );

    auto hubState = CommonUtils::HubState::fromJson( request[ "config" ][ "software" ] );
    for( auto& pluginSummary : hubState.plugins ) {
        m_pluginIds.emplace_back( pluginSummary.id );
    }
}

BackupHelper::~BackupHelper() {
    if( m_error.empty() ) {
        LOGGER_INFO_RAW( cc::info( "Backup successful" ) );
    }
    else {
        LOGGER_ERROR_RAW( cc::error( "Backup failed" ) );
    }
}

bool BackupHelper::backupDatabase( const std::string& tableName,
                                   const ProvisionDataUtils::ProvisionData& provisionData,
                                   const std::string& decryptField ) {
    if( decryptField.empty() ) {
        return backupDatabase( tableName );
    }

    return backupDatabase( tableName, [&]( Json& network ) {
        if( network.count( decryptField ) ) {
            network[ decryptField ] = provisionData.decryptSensitiveData( json_get_str( network, decryptField ) );
        }
    } );
}

bool BackupHelper::backupDatabase( const std::string& tableName, const EntryPredicate& entryPredicate ) {
    if( !canBackupToDirectory( "db" ) ) {
        return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Backuping table " ) + cc::note( tableName ) );

    auto backupData = JSON_EMPTY_OBJ;
    auto records = m_dbClient.findAll( tableName, JSON_EMPTY_OBJ, nullptr );

    for( auto& record : records ) {
        entryPredicate( record );
    }

    backupData[ "data" ] = std::move( records );

    const std::string fileName = m_backupDirectory + "/db/" + tableName + ".json";
    if( !encryptAndBackup( fileName, backupData.dump() ) ) {
        m_error = "Failed to backup table " + tableName;
        LOGGER_ERROR_RAW( cc::error( "Failed to backup table " ) + cc::note( tableName ) );
        return false;
    }

    return true;
}

bool BackupHelper::backupFile( const std::string& filePath ) {
    if( !canBackupToDirectory( "data" ) ) {
        return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Backuping file " ) + cc::note( filePath ) );

    std::string backupData;
    if( !tiny::tools::file2string( ( filePath ).c_str(), backupData ) ) {
        m_error = "Failed to read file " + filePath;
        LOGGER_ERROR_RAW( cc::error( "Failed to read file " ) + cc::note( filePath ) );
        return false;
    }

    const std::string backupFileName = m_backupDirectory + "/data/" + filesystem::getFilenameWithExtension( filePath );
    if( !encryptAndBackup( backupFileName, backupData ) ) {
        m_error = "Failed to backup file " + filePath;
        LOGGER_ERROR_RAW( cc::error( "Failed to backup file " ) + cc::note( filePath ) );
        return false;
    }

    return true;
}

bool BackupHelper::backupJson( const std::string& path, const Json& data ) {
    std::string backupPath;
    auto filePos = path.find_last_of( '/' );
    if( filePos != std::string::npos ) {
        backupPath = path.substr( 0, filePos );
    }

    if( !canBackupToDirectory( backupPath ) ) {
        return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Backuping data at " ) + cc::note( path ) );

    const std::string fileName = m_backupDirectory + "/" + path + ".json";

    if( filesystem::isFileExist( fileName ) ) {
        m_error = "File with specified path already exist";
        LOGGER_ERROR_RAW( cc::error( "File with specified path already exist" ) );
        return false;
    }

    if( !encryptAndBackup( fileName, data.dump() ) ) {
        m_error = "Failed to backup data at " + path;
        LOGGER_ERROR_RAW( cc::error( "Failed to backup data at " ) + cc::note( path ) );
        return false;
    }

    return true;
}

bool BackupHelper::canBackupToDirectory( const std::string& path ) {
    if( m_backupDirectory.empty() ) {
        m_error = "Backup directory empty";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        return false;
    }

    auto targetDir = m_backupDirectory + ( path.empty() ? "" : "/" + path );
    if( !filesystem::createFolder( targetDir ) ) {
        m_error = "Can not create directory " + path + " for backup";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        return false;
    }
    return true;
}

bool BackupHelper::encryptAndBackup( const std::string& path, const std::string& data ) {
    std::string encrypted;
    if( !m_crypt.encryptData( data, encrypted ) ) {
        return false;
    }
    std::ofstream outFile( path, std::ofstream::trunc );
    if( outFile ) {
        outFile << encrypted;
    }
    outFile.close();
    return !!outFile;
}

const std::vector< std::string >& BackupHelper::getPluginIds() const {
    return m_pluginIds;
}

std::string BackupHelper::getBackupDataDirectory() const {
    return m_backupDirectory + "/data";
}

Json BackupHelper::getResponse() const {
    if( !m_error.empty() ) {
        return createFailedResponse( m_error );
    }

    return createSuccessResponse();
}

Json BackupHelper::createFailedResponse( const std::string& reason ) {
    auto response = JSON_EMPTY_OBJ;
    response[ "success" ] = false;
    setError( response, Errors::BACKUP_CREATE_FAILED, reason );
    return response;
}

Json BackupHelper::createSuccessResponse() {
    auto response = JSON_EMPTY_OBJ;
    response[ "success" ] = true;
    return response;
}

}  // namespace CommonUtils
}  // namespace HubPlatform
