
#include "backupd/remote_interface/backup_remote_interface.h"
#include "backupd/resources/backup_resources.h"

#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"

#include "base_utils/ezlodb.h"
#include "base_utils/acl.h"
#include "database_client/dbqueryparser.h"
#include "service_interfaces/service.h"

#include <unordered_set>
#include <cassert>

namespace HubPlatform {
namespace BackupUpdateManager {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

void RemoteInterface::processCmd( const std::string& backupMethod, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( RemoteInterface::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "create", &RemoteInterface::handleBackupCreate },
        { "restore", &RemoteInterface::handleBackupRestore },
        { "list", &RemoteInterface::handleGetList },
    };

    const auto it = requestHandlers.find( backupMethod );
    if( it != requestHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

Errors::Error RemoteInterface::backupModuleDatabase( const std::string& tempBackupFilesDirectory,
                                                     const std::string& serviceName,
                                                     const std::string& version,
                                                     const std::vector< std::string >& tableNames ) {
    if( tableNames.empty() ) {
        return Errors::BACKUP_CREATE_TABLE_EMPTY;
    }
    auto backupData = JSON_EMPTY_OBJ;
    for( auto const& tableName : tableNames ) {
        auto records =
            getDBClient().findAll( tableName, JSON_EMPTY_OBJ, nullptr, std::numeric_limits< int >::max(), 0 );
        backupData[ tableName ] = std::move( records );
    }
    backupData[ "version" ] = version;
    const std::string fileName =
        tempBackupFilesDirectory + Resources::FileNamePrefix + serviceName + Resources::BackupFileExtension;
    std::ofstream outfile( fileName, std::ios::out );
    if( !outfile.good() ) {
        LOGGER_WARN( serviceName + ": " + "file " + fileName + " is invalid" );
        return Errors::ERROR_OPENING_FILE;
    }
    outfile << backupData << std::endl;
    outfile.close();
    LOGGER_INFO( serviceName + ": " + "success" );
    return Errors::OK;
}

Errors::Error RemoteInterface::restoreModuleDatabase( const std::string& tempBackupFilesDirectory,
                                                      const std::string& serviceName,
                                                      const std::string& version ) {
    const std::string fileName =
        tempBackupFilesDirectory + Resources::FileNamePrefix + serviceName + Resources::BackupFileExtension;
    std::ifstream file( fileName );
    if( !file.good() ) {
        LOGGER_WARN( serviceName + ": " + "file " + fileName + " is invalid" );
        return Errors::ERROR_OPENING_FILE;
    }
    JsonObject restoredData;
    try {
        file >> restoredData;
    }
    catch( std::exception& e ) {
        LOGGER_WARN( serviceName + ": " + "cann't parse json from file: " + e.what() );
        return Errors::JSON_PARSE_ERROR;
    }
    auto it = restoredData.find( "version" );
    if( it == restoredData.end() || *it != version ) {
        LOGGER_WARN( serviceName + ": " + "wrong backup version" );
        return Errors::WRONG_BACKUP_VERSION;
    }
    restoredData.erase( it.key() );
    for( JsonObject::iterator it = restoredData.begin(); it != restoredData.end(); ++it ) {
        getDBClient().remove( it.key(), JSON_EMPTY_OBJ );
        const JsonArray& array = it.value();
        for( const auto& json : array ) {
            getDBClient().update( it.key(), JSON_EMPTY_OBJ, json, true, false );
        }
    }
    LOGGER_INFO( serviceName + ": " + "success" );
    return Errors::OK;
}

Errors::Error RemoteInterface::recoverModuleDatabase( const std::string& tempBackupFilesDirectory,
                                                      const std::string& serviceName,
                                                      const std::string& version ) {
    LOGGER_INFO( serviceName );
    return restoreModuleDatabase( tempBackupFilesDirectory, serviceName + "_tmp", version );
}

Errors::Error RemoteInterface::emergencyBackup( const std::string& tempBackupFilesDirectory,
                                                const std::string& serviceName,
                                                const std::string& version,
                                                const std::vector< std::string >& tableNames ) {
    LOGGER_INFO( serviceName );
    return backupModuleDatabase( tempBackupFilesDirectory, serviceName + "_tmp", version, tableNames );
}

std::string RemoteInterface::getTempBackupDirectory( const JsonObject& request, JsonObject& response ) {
    std::string tempBackupFilesDirectory;
    if( !getField( request, "directory", response, tempBackupFilesDirectory ) ) {
        return "";
    }
    return tempBackupFilesDirectory;
}

std::string RemoteInterface::composeFilePath( const std::string& tempBackupFilesDirectory,
                                              const std::string& serviceName,
                                              const std::string& originFileName,
                                              const std::string& prefix ) {
    if( prefix.empty() ) {
        return tempBackupFilesDirectory + Resources::FileNamePrefix + serviceName + "_" + originFileName;
    }
    else {
        return tempBackupFilesDirectory + prefix + serviceName + "_" + originFileName;
    }
}

void RemoteInterface::handleBackupCreate( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    auto response = m_hostService.sendRequest( "backupd", rpcMessage.getParams(), BACKUP_CREATE );
    setErrorIfHas( joAnswer, response );
    joAnswer[ "result" ] = std::move( response[ "result" ] );
}

void RemoteInterface::handleBackupRestore( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    auto response = m_hostService.sendRequest( "backupd", rpcMessage.getParams(), BACKUP_RESTORE );
    setErrorIfHas( joAnswer, response );
    joAnswer[ "result" ] = std::move( response[ "result" ] );
}

void RemoteInterface::handleGetList( rpc::Message& rpcMessage ) {
    auto& joAnswer = rpcMessage.getAnswer();
    auto peer = rpcMessage.getNetworkObject();
    m_hostService.sendAsyncRequest( "backupd",
                                    rpcMessage.getParams(),
                                    BACKUP_LIST,
                                    [joAnswer, peer]( const JsonObject& response ) mutable {
                                        setErrorIfHas( joAnswer, response );
                                        joAnswer[ "result" ] = std::move( response );
                                        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );

                                        if( peer ) {
                                            peer->sendMessage( joAnswer );
                                        }
                                    },
                                    std::chrono::minutes( 5 ) );
    rpcMessage.getAnswer() = nullptr;
}

IDbClient& RemoteInterface::getDBClient() {
    return m_hostService.getDBClient();
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
