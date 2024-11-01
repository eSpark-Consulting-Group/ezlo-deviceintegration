#include "common_utils/backup/restore_helper.h"
#include "common_utils/hub_state.h"

#include "system_utils/filesystem.h"

#include "jsonrpc/jsonrpc.h"
#include "tiny/tiny_process.h"

#include <fstream>

namespace HubPlatform {
namespace CommonUtils {

RestoreHelper::RestoreHelper( IDbClient& dbClient, std::string backupDirectory )
: m_dbClient( dbClient )
, m_backupDirectory( std::move( backupDirectory ) )
, m_returnCode( ReturnCode::Ok ) {
    LOGGER_INFO_RAW( cc::info( "Starting backup restore" ) );

    if( m_backupDirectory.empty() ) {
        LOGGER_ERROR_RAW( cc::error( "Intermediate backup directory is empty" ) );
        m_returnCode = ReturnCode::InvalidArguments;
    }

    LOGGER_TRACE_RAW( cc::trace( "Intermediate backup directory: " ) + cc::p( m_backupDirectory ) );

    if( !restoreJson( "config", m_config ) ) {
        return;
    }

    m_crypt.setVersion( json_get_str( m_config, "version" ) );
    auto hubState = CommonUtils::HubState::fromJson( m_config[ "software" ] );
    for( auto& pluginSummary : hubState.plugins ) {
        m_pluginIds.emplace_back( pluginSummary.id );
    }
}

RestoreHelper::~RestoreHelper() {
    if( m_returnCode != ReturnCode::Ok ) {
        LOGGER_ERROR_RAW( cc::error( "Backup restore failed: " ) + returnCodeToString( m_returnCode ) );
    }
    else {
        LOGGER_INFO_RAW( cc::info( "Backup restore successful" ) );
    }
}

bool RestoreHelper::restoreDatabase( const std::string& tableName,
                                     const ProvisionDataUtils::ProvisionData& provisionData,
                                     const std::string& encryptField ) {
    if( encryptField.empty() ) {
        return restoreDatabase( tableName );
    }

    return restoreDatabase( tableName, [&]( Json& network ) {
        if( network.count( encryptField ) ) {
            auto value = json_get_str( network, encryptField );
            if( !value.empty() ) {
                network[ encryptField ] = provisionData.encryptSensitiveData( json_get_str( network, encryptField ) );
            }
        }
    } );
}

bool RestoreHelper::restoreDatabase( const std::string& tableName, const EntryPredicate& entryPredicate ) {
    if( m_returnCode != ReturnCode::Ok ) {
        return false;
    }

    if( tableName.empty() ) {
        m_returnCode = ReturnCode::InvalidArguments;
        LOGGER_ERROR_RAW( cc::error( "Table name is empty" ) );
        return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Restoring table " ) + cc::note( tableName ) );

    const std::string backupFileName = m_backupDirectory + "/db/" + tableName + ".json";
    if( !filesystem::isExist( backupFileName ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Table " ) + cc::note( tableName ) + cc::debug( " not found. Skipping ... " ) );
        return true;
    }

    std::string backupData;
    if( !readAndDecrypt( backupFileName, backupData ) ) {
        m_returnCode = ReturnCode::BackupDataReadFailed;
        LOGGER_ERROR_RAW( cc::error( "Failed to read and decrypt table " ) + cc::note( tableName ) );
        return false;
    }

    if( backupData.empty() ) {
        m_returnCode = ReturnCode::BackupDataEmpty;
        LOGGER_ERROR_RAW( cc::error( "Data is empty" ) );
        return false;
    }

    Json restoredData;
    try {
        restoredData = Json::parse( backupData );
    }
    catch( ... ) {
        m_returnCode = ReturnCode::BackupDataParseFailed;
        LOGGER_ERROR_RAW( cc::error( "Failed to parse data" ) );
        return false;
    }

    JsonArray items;
    if( !json_get_arr( restoredData, "data", items ) ) {
        m_returnCode = ReturnCode::BackupDataFormatIncorrect;
        LOGGER_ERROR_RAW( cc::error( "Incorrect data format" ) );
        return false;
    }

    m_dbClient.clear( tableName );
    for( auto& item : items ) {
        entryPredicate( item );
        m_dbClient.insert( tableName, item );
    }
    return true;
}

bool RestoreHelper::restoreFile( const std::string& filePath ) {
    if( m_returnCode != ReturnCode::Ok ) {
        return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Restoring file " ) + cc::note( filePath ) );

    const std::string backupFileName = m_backupDirectory + "/data/" + filesystem::getFilenameWithExtension( filePath );

    std::string backupData;
    if( !readAndDecrypt( backupFileName, backupData ) ) {
        m_returnCode = ReturnCode::BackupDataReadFailed;
        LOGGER_ERROR_RAW( cc::error( "Failed to read and decrypt backuped file " ) + cc::note( filePath ) );
        return false;
    }

    std::ofstream outFile( filePath, std::ofstream::trunc );
    if( outFile ) {
        outFile << backupData;
    }
    outFile.close();
    if( !outFile ) {
        m_returnCode = ReturnCode::RestoredDataWriteFailed;
        LOGGER_ERROR_RAW( cc::error( "Failed to write file " ) + cc::note( filePath ) );
        return false;
    }

    return true;
}

bool RestoreHelper::restoreJson( const std::string& path, Json& data ) {
    if( m_returnCode != ReturnCode::Ok ) {
        return false;
    }

    if( path.empty() ) {
        m_returnCode = ReturnCode::InvalidArguments;
        LOGGER_ERROR_RAW( cc::error( "Path is empty" ) );
        return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Restoring data from " ) + cc::note( path ) );

    const std::string backupFileName = m_backupDirectory + "/" + path + ".json";

    std::string backupData;
    if( !readAndDecrypt( backupFileName, backupData ) ) {
        m_returnCode = ReturnCode::BackupDataReadFailed;
        LOGGER_ERROR_RAW( cc::error( "Failed to read and decrypt backuped " ) + cc::note( path ) );
        return false;
    }

    if( backupData.empty() ) {
        m_returnCode = ReturnCode::BackupDataEmpty;
        LOGGER_ERROR_RAW( cc::error( "Data is empty" ) );
        return false;
    }

    try {
        data = Json::parse( backupData );
        return true;
    }
    catch( const std::exception& ex ) {
        m_returnCode = ReturnCode::BackupDataParseFailed;
        LOGGER_ERROR_RAW( cc::error( "Failed to parse data: " ) + ex.what() );
        return false;
    }
}

std::string RestoreHelper::getBackupDataDirectory() const {
    return m_backupDirectory + "/data";
}

const Json& RestoreHelper::getConfig() const {
    return m_config;
}

const std::vector< std::string >& RestoreHelper::getPluginIds() const {
    return m_pluginIds;
}

bool RestoreHelper::readAndDecrypt( const std::string& path, std::string& data ) const {
    std::string encrypted;
    if( !tiny::tools::file2string( ( path ).c_str(), encrypted ) ) {
        return false;
    }
    return m_crypt.decryptData( encrypted, data );
}

int RestoreHelper::getReturnCode() const {
    return static_cast< int >( m_returnCode );
}

void RestoreHelper::setReturnCode( int code ) {
    m_returnCode = static_cast< ReturnCode >( code );
}

std::string RestoreHelper::returnCodeToString( ReturnCode returnCode ) {
    switch( returnCode ) {
    case ReturnCode::Ok:
        return "";
    case ReturnCode::InvalidArguments:
        return "Invalid arguments";
    case ReturnCode::BackupDataReadFailed:
        return "Backup data read failed";
    case ReturnCode::BackupDataEmpty:
        return "Backup data empty";
    case ReturnCode::BackupDataParseFailed:
        return "Backup data parse failed";
    case ReturnCode::BackupDataFormatIncorrect:
        return "Backup data format incorrect";
    case ReturnCode::RestoredDataWriteFailed:
        return "Restored data write failed";
    case ReturnCode::UnknownError:
        return "Unknown error";
    default:
        return "Error with code: " + std::to_string( static_cast< int >( returnCode ) );
    }
}

}  // namespace CommonUtils
}  // namespace HubPlatform
