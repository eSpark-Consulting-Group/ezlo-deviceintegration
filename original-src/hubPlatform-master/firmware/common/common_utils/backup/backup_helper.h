#pragma once

#include "common_utils/backup/backup_restore_crypt.h"

#include "provision_data_utils/provision_data.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CommonUtils {

class BackupHelper final {
public:
    BackupHelper( IDbClient& dbClient, const JsonObject& request );
    ~BackupHelper();

    using EntryPredicate = std::function< void( Json& entry ) >;

    bool backupDatabase( const std::string& tableName,
                         const ProvisionDataUtils::ProvisionData& provisionData,
                         const std::string& decryptField );
    bool backupDatabase( const std::string& tableName, const EntryPredicate& entryPredicate = []( Json& ) {} );
    bool backupFile( const std::string& filePath );
    bool backupJson( const std::string& path, const Json& data );

    const std::vector< std::string >& getPluginIds() const;

    std::string getBackupDataDirectory() const;

    // Returns failed response if there was errors in this helper methods
    Json getResponse() const;

    // Can be used if there is other logic that can fail outside this helper
    static Json createFailedResponse( const std::string& reason );
    static Json createSuccessResponse();

private:
    bool canBackupToDirectory( const std::string& path );
    bool encryptAndBackup( const std::string& path, const std::string& data );

private:
    IDbClient& m_dbClient;
    std::string m_backupDirectory;
    BackupRestoreCrypt m_crypt;
    std::vector< std::string > m_pluginIds;

    std::string m_error;
};

}  // namespace CommonUtils
}  // namespace HubPlatform
