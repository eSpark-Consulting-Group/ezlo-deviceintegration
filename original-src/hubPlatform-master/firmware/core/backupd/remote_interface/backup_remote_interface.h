#pragma once

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

class IDbClient;

namespace HubPlatform {
namespace Services {
class Service;
}
namespace BackupUpdateManager {

class RemoteInterface final {
public:
    RemoteInterface( Services::Service& hostService );

    void processCmd( const std::string& backupMethod, rpc::Message& rpcMessage );

    Errors::Error backupModuleDatabase( const std::string& tempBackupFilesDirectory,
                                        const std::string& serviceName,
                                        const std::string& version,
                                        const std::vector< std::string >& tableNames );
    Errors::Error restoreModuleDatabase( const std::string& tempBackupFilesDirectory,
                                         const std::string& serviceName,
                                         const std::string& version );
    Errors::Error recoverModuleDatabase( const std::string& tempBackupFilesDirectory,
                                         const std::string& serviceName,
                                         const std::string& version );
    Errors::Error emergencyBackup( const std::string& tempBackupFilesDirectory,
                                   const std::string& serviceName,
                                   const std::string& version,
                                   const std::vector< std::string >& tableNames );
    std::string getTempBackupDirectory( const JsonObject& request, JsonObject& response );
    std::string composeFilePath( const std::string& tempBackupFilesDirectory,
                                 const std::string& serviceName,
                                 const std::string& originFileName,
                                 const std::string& prefix = std::string() );

    IDbClient& getDBClient();

private:
    void handleBackupCreate( rpc::Message& rpcMessage );
    void handleBackupRestore( rpc::Message& rpcMessage );
    void handleGetList( rpc::Message& rpcMessage );

private:
    Services::Service& m_hostService;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
