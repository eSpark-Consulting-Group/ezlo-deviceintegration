#pragma once

#include "common_utils/backup/backup_restore_crypt.h"

#include "provision_data_utils/provision_data.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CommonUtils {

class RestoreHelper final {
public:
    enum class ReturnCode {
        Ok = 0,
        InvalidArguments,
        BackupDataReadFailed,
        BackupDataEmpty,
        BackupDataParseFailed,
        BackupDataFormatIncorrect,
        RestoredDataWriteFailed,

        UnknownError = 255
    };

    RestoreHelper( IDbClient& dbClient, std::string backupDirectory );
    ~RestoreHelper();

    using EntryPredicate = std::function< void( Json& entry ) >;

    bool restoreDatabase( const std::string& tableName,
                          const ProvisionDataUtils::ProvisionData& provisionData,
                          const std::string& encryptField );
    bool restoreDatabase( const std::string& tableName, const EntryPredicate& entryPredicate = []( Json& ) {} );
    bool restoreFile( const std::string& filePath );
    bool restoreJson( const std::string& path, Json& data );

    std::string getBackupDataDirectory() const;

    const Json& getConfig() const;
    const std::vector< std::string >& getPluginIds() const;

    // Returns failed result if there was errors in this helper methods
    int getReturnCode() const;
    void setReturnCode( int code );

    static std::string returnCodeToString( ReturnCode returnCode );

private:
    bool readAndDecrypt( const std::string& path, std::string& data ) const;

private:
    IDbClient& m_dbClient;
    std::string m_backupDirectory;
    BackupRestoreCrypt m_crypt;

    Json m_config;
    std::vector< std::string > m_pluginIds;

    ReturnCode m_returnCode;
};

}  // namespace CommonUtils
}  // namespace HubPlatform
