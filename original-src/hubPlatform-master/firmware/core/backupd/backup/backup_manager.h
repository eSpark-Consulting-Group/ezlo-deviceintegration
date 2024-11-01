#pragma once

#include "service_interfaces/service.h"
#include "backupd/backup_messenger.h"
#include "backupd/backup/backup_script_manager.h"
#include "backupd/backup/iglobal_lock_manager.h"
#include "backupd/backup/ivera_backup_manager.h"
#include "cloud_storage/cloud_storage_type.h"

#include <unordered_set>
#include <mutex>

class IDbClient;

namespace HubPlatform {
namespace BackupUpdateManager {

class Messenger;

class BackupManager {
public:
    using UploadResultCallback = std::function<
        void( const Json& userData, const std::string& error, const Common::CloudStorageInterface::FileKey& fileKey ) >;
    using UploadFileCallback = std::function< bool( const std::string&, const Json&, UploadResultCallback ) >;
    using DownloadResultCallback = std::function< void( const Json& userData, const std::string& error ) >;
    using DownloadFileCallback = std::function<
        bool( const Common::CloudStorageInterface::FileKey&, const std::string&, DownloadResultCallback ) >;

    BackupManager( IGlobalLockManager& backupLockManager,
                   IGlobalLockManager& restoreLockManager,
                   Messenger& messenger,
                   ScriptManagerBase& scriptManager,
                   IVeraBackupManager& veraBackupManager,
                   IDbClient& dbClient,
                   Services::TaskLoop& taskLoop,
                   UploadFileCallback uploadFile,
                   DownloadFileCallback downloadFile,
                   std::chrono::seconds delayOnFailedLoad = std::chrono::seconds( 60 ) );

    virtual ~BackupManager() = default;
    void setUp();
    void create();
    void restore( const Common::CloudStorageInterface::FileKey& key );
    const std::string& getBackupDirectory() const;

public:
    Messenger& getMessenger();
    IDbClient& getDbClient();

private:
    void onBackupLocked( const std::string& error );
    void onBackupUnlocked();
    void onBackupServiceRestartedDuringLock( const std::string& service );

    void handleBackupResponse( const std::string& service, int operationId, const JsonObject& response );
    void uploadBackup( int attempts );

private:
    void downloadBackup( int attempts );
    void startRestore();

    void onRestoreLocked( const std::string& error );
    void onRestoreUnlocked();
    void onRestoreServiceRestartedDuringLock( const std::string& service );

    bool isBackupHubStateCompatible();
    std::string restoreData( const std::string& intermediateBackupDirectory );
    bool performRecovery();

    void clearBackupDirectory();

    void prepareCreateBackup( IGlobalLockManager& lockManager,
                              std::function< void( const std::string&, int, const JsonObject& ) > responseCallback );

    void handleRestoreResponse( const std::string& service, int operationId, const JsonObject& response );

    std::string generateBackupRecoveryDirectory() const;

    bool parseEzloRestoreConfig();
    bool isVeraBackup();
    bool checkAndCalculateMD5Checksum();

    void performEzloRestore();
    void performVeraRestore();
    void onVeraRestoreFinished( std::string error );

    std::string getDatabaseVersion();
    int getNumberOfDevices();

private:
    IGlobalLockManager& m_backupLockManager;
    IGlobalLockManager& m_restoreLockManager;
    Messenger& m_messenger;
    ScriptManagerBase& m_scriptManager;
    IVeraBackupManager& m_veraBackupManager;
    IDbClient& m_dbClient;
    Services::TaskLoop& m_taskLoop;
    UploadFileCallback m_uploadFile;
    DownloadFileCallback m_downloadFile;
    std::chrono::seconds m_delayOnFailedLoad;
    std::string m_backupDirectory;

    static const std::vector< std::pair< std::string, std::string > > m_backupServices;
    static int m_operationIdCounter;
    int m_pendingOperationId;
    std::string m_error;

    std::chrono::high_resolution_clock::duration m_backupTimestamp;
    std::string m_intermediateBackupDirectory;
    std::string m_backupArchivePath;

    std::string m_restoreArchivePath;
    std::string m_intermediateRestoreDirectory;

    Json m_backupConfig;
    Json m_restoreConfig;
    std::string m_restoreType;

    std::set< std::string > m_backupedServices;
    int m_unavailableAddons;

    Json m_backupMeta;
    Common::CloudStorageInterface::FileKey m_backupKey;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
