#include "backupd/backup/backup_manager.h"
#include "backupd/resources/backup_resources.h"

#include "log_utils/ezlo_log.h"
#include "base_utils/md5.h"
#include "base_utils/return_action.h"
#include "base_utils/ezlo_common.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/hub_info_helper.h"

#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "crypto_utils/digests.h"

#include <future>
#include <fstream>

namespace HubPlatform {
namespace BackupUpdateManager {

namespace {
const auto UploadAttempts = 3;
const auto DownloadAttempts = 3;
}  // namespace

// NOTE: keep this list in sync with each service backup logic.
const std::vector< std::pair< std::string, std::string > > BackupManager::m_backupServices = {
    { "cored", "ha-cored" },
    { "extensions_manager", "ha-extensions_manager" },
    { "external_controllers", "ha-external_controllers" },
    { "hardware_manager", "ha-hardware_manager" },
    { "network", "ha-networkd" },
    { "scenes_manager", "ha-scenes_manager" },
    { "ui_service", "ha-uid" },
    { "cloud_devices", "ha-cloud_devices" },
    { "addon.2gig", "addons/ha-2gigd" },
    { "addon.cameras", "addons/ha-camerasd" },
    { "addon.zigbee", "addons/ha-zigbeed" },
    { "addon.zwave", "addons/ha-zwaved" },
};

int BackupManager::m_operationIdCounter = 0;

BackupManager::BackupManager( IGlobalLockManager& backupLockManager,
                              IGlobalLockManager& restoreLockManager,
                              Messenger& messenger,
                              ScriptManagerBase& scriptManager,
                              IVeraBackupManager& veraBackupManager,
                              IDbClient& dbClient,
                              Services::TaskLoop& taskLoop,
                              UploadFileCallback uploadFile,
                              DownloadFileCallback downloadFile,
                              std::chrono::seconds delayOnFailedLoad )
: m_backupLockManager( backupLockManager )
, m_restoreLockManager( restoreLockManager )
, m_messenger( messenger )
, m_scriptManager( scriptManager )
, m_veraBackupManager( veraBackupManager )
, m_dbClient( dbClient )
, m_taskLoop( taskLoop )
, m_uploadFile( std::move( uploadFile ) )
, m_downloadFile( std::move( downloadFile ) )
, m_delayOnFailedLoad( delayOnFailedLoad )
, m_backupDirectory( Environment::getDataFolder() + "/" + Resources::BackupsDirectoryName )
, m_unavailableAddons( 0 )
, m_pendingOperationId( 0 )
, m_backupTimestamp( 0 ) {
    filesystem::createFolder( m_backupDirectory );
}

void BackupManager::setUp() {
    // In case we recovered from restart
    m_error = "Service restarted";
    m_backupLockManager.start(
        [this]( const std::string& error ) { onBackupLocked( error ); },
        [this]() { onBackupUnlocked(); },
        [this]( const std::string& service ) { onBackupServiceRestartedDuringLock( service ); } );

    bool ok = m_restoreLockManager.start(
        [this]( const std::string& error ) { onRestoreLocked( error ); },
        [this]() { onRestoreUnlocked(); },
        [this]( const std::string& service ) { onRestoreServiceRestartedDuringLock( service ); } );

    m_veraBackupManager.setUp( [this]( const std::string& error ) {
        getDbClient().clear( TempVeraDeviceMappingsDBName );
        onVeraRestoreFinished( error );
    } );

    // NOTE: If restore lock manager is ok during start but we have recovery data,
    //       we were restarted during restore when data may be corrupted, so need to run recovery
    if( ok && filesystem::isDirectoryExist( generateBackupRecoveryDirectory() ) ) {
        m_error = "Backup service restarted during restoring backup";
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );

        ezlo::common::APIs::file2json( ( generateBackupRecoveryDirectory() + "/config.json" ).c_str(),
                                       m_restoreConfig );
        performRecovery();
        // NOTE: other services may not be started at this moment, assume we can not fail starting daemons
        m_scriptManager.startDaemons();
    }
    clearBackupDirectory();
}

Messenger& BackupManager::getMessenger() {
    return m_messenger;
}

IDbClient& BackupManager::getDbClient() {
    return m_dbClient;
}

// Backup stages progress distribution
//  lock   -  20% (20%)
//  create -  50% (30%)
//  unlock -  60% (10%)
//  upload - 100% (40%)
void BackupManager::create() {
    LOGGER_INFO_RAW( cc::info( "Prepare for creating backup" ) );

    if( m_pendingOperationId || m_backupLockManager.isLocked() || m_restoreLockManager.isLocked() ) {
        LOGGER_ERROR_RAW( cc::error( "Other operation in progress" ) );
        return;
    }

    m_backupTimestamp = std::chrono::high_resolution_clock::now().time_since_epoch();

    m_pendingOperationId = ++m_operationIdCounter;
    m_error = "";
    m_intermediateBackupDirectory =
        getBackupDirectory() + "/" + Resources::FileNamePrefix + std::to_string( m_backupTimestamp.count() );
    m_backupArchivePath = m_intermediateBackupDirectory + ".tgz";

    getMessenger().sendProgressStatus( BACKUP_PROGRESS, "started", 0 );

    if( !m_backupLockManager.lock() ) {
        m_error = "Failed to start locking firmware";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( BACKUP_PROGRESS, Errors::BACKUP_CREATE_FAILED, m_error );
        m_pendingOperationId = 0;
        return;
    }

    // Waiting for onLocked method to be called
}

void BackupManager::prepareCreateBackup(
    IGlobalLockManager& lockManager,
    std::function< void( const std::string&, int, const JsonObject& ) > responseCallback ) {
    // Create intermediate directory
    if( !filesystem::createFolder( m_intermediateBackupDirectory ) ||
        !filesystem::clearFolder( m_intermediateBackupDirectory ) ) {
        m_error = "Failed to create intermediate backup directory";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        lockManager.unlock();
        return;
    }

    auto hubState = lockManager.getHubState();
    m_backupConfig = JSON_EMPTY_OBJ;
    m_backupConfig[ "version" ] = Resources::BackupVersion;
    m_backupConfig[ "hub_type" ] = HubInfoHelper::getHubType();
    m_backupConfig[ "serial" ] = HubInfoHelper::getSerial();
    m_backupConfig[ "type" ] = Resources::BackupTypeNative;
    m_backupConfig[ "software" ] = hubState.toJson();

    {
        std::ofstream configFile( m_intermediateBackupDirectory + "/config.json", std::ofstream::trunc );
        if( configFile ) {
            configFile << m_backupConfig;
        }
        configFile.close();
        if( !configFile ) {
            m_error = "Failed to write backup config";
            LOGGER_ERROR_RAW( cc::error( m_error ) );
            lockManager.unlock();
            return;
        }
        // NOTE: create basic directory structure beforehand
        filesystem::createFolder( m_intermediateBackupDirectory + "/db" );
        filesystem::createFolder( m_intermediateBackupDirectory + "/data" );
    }

    std::set< std::string > availableAddons;
    for( auto& addon : hubState.addons ) {
        auto addonService = "addon." + addon.id;
        availableAddons.insert( addonService );
    }

    m_backupedServices.clear();
    m_unavailableAddons = 0;
    for( const auto& servicePair : m_backupServices ) {
        auto& service = servicePair.first;
        if( service.find( "addon." ) == 0U && availableAddons.find( service ) == availableAddons.end() ) {
            LOGGER_WARN_RAW( cc::warn( "Addon " ) + cc::note( service ) + cc::warn( " is not available" ) );
            m_unavailableAddons++;
            continue;
        }
        LOGGER_DEBUG_RAW( cc::debug( "Sending backup request to " ) + cc::note( service ) + cc::debug( " service" ) );
        getMessenger().sendBackupRequest(
            service,
            m_intermediateBackupDirectory,
            m_backupConfig,
            std::bind( responseCallback, service, m_pendingOperationId, std::placeholders::_1 ) );
    }

    // Waiting for handleBackupResponse methods to be called for each required service
}

void BackupManager::onBackupLocked( const std::string& error ) {
    if( !error.empty() ) {
        m_error = "Failed to lock firmware";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        m_error += ": " + error;
        // Wait for onUnlocked for reporting this error
        return;
    }

    LOGGER_INFO_RAW( cc::info( "Start creating backup" ) );
    getMessenger().sendProgressStatus( BACKUP_PROGRESS, "process", 20 );

    prepareCreateBackup( m_backupLockManager,
                         std::bind( &BackupManager::handleBackupResponse,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3 ) );
}

void BackupManager::handleBackupResponse( const std::string& service, int operationId, const JsonObject& response ) {
    if( m_pendingOperationId != operationId ) {
        LOGGER_DEBUG_RAW( cc::debug( "Got outdated response on backup request from " ) + cc::note( service ) +
                          cc::debug( " service" ) );
        // Ignore this response, looks like we've canceled this backup already
        return;
    }

    if( hasError( response ) || !json_get_bool( response, "success", false ) ) {
        std::string message = json_get_str( response[ "error" ], "message" );
        std::string reason = json_get_str( response[ "error" ], "reason" );
        reason = message.empty() ? "Unknown reason" : message + ( reason.empty() ? "" : ": " + reason );
        LOGGER_ERROR_RAW( cc::error( "Service " ) + cc::note( service ) + cc::error( " failed to create backup: " ) +
                          cc::note( reason ) );
        m_error = "Service " + service + " failed to create backup: " + reason;
        m_backupLockManager.unlock();
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Service " ) + cc::note( service ) + cc::debug( " backuped successfully" ) );
    m_backupedServices.insert( service );

    int leftServices = m_backupServices.size() - m_unavailableAddons - m_backupedServices.size();
    if( leftServices != 0 ) {
        LOGGER_TRACE_RAW( cc::trace( "Waiting for " ) + cc::num10( leftServices ) + cc::trace( " more response(s)" ) );
        return;
    }

    if( !m_error.empty() ) {
        return;
    }

    // All services are backuped
    LOGGER_DEBUG_RAW( cc::debug( "All services backuped successfully" ) );
    LOGGER_DEBUG_RAW( cc::debug( "Creating backup archive" ) );
    getMessenger().sendProgressStatus( BACKUP_PROGRESS, "process", 50 );

    const std::string command = "cd " + m_intermediateBackupDirectory + " && tar -zcf " + m_backupArchivePath + " *";

    if( std::system( command.c_str() ) != 0 ) {
        m_error = "Failed to create backup archive";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        m_backupLockManager.unlock();
        return;
    }
    if( !filesystem::deleteFolder( m_intermediateBackupDirectory ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to delete intermediate backup directory" ) );
    }

    m_backupLockManager.unlock();
    // Waiting for onUnlocked method to be called
}

void BackupManager::onBackupUnlocked() {
    if( !m_error.empty() ) {
        m_pendingOperationId = 0;
        clearBackupDirectory();
        getMessenger().sendProgressError( BACKUP_PROGRESS, Errors::BACKUP_CREATE_FAILED, m_error );
        m_error = "";
        return;
    }

    // TODO: we could send archive asynchronously to unlock
    getMessenger().sendProgressStatus( BACKUP_PROGRESS, "process", 60 );
    LOGGER_INFO_RAW( cc::info( "Starting backup archive upload" ) );

    Json controllerInfo;
    controllerInfo[ "controller_uuid" ] = HubInfoHelper::getControllerUUID();
    controllerInfo[ "timestamp" ] = std::chrono::duration_cast< std::chrono::seconds >( m_backupTimestamp ).count();
    controllerInfo[ "config" ] = m_backupConfig;
    controllerInfo[ "database_version" ] = getDatabaseVersion();
    controllerInfo[ "number_of_devices" ] = getNumberOfDevices();

    m_backupMeta = JSON_EMPTY_OBJ;
    m_backupMeta[ "controller_info" ] = controllerInfo;

    uploadBackup( UploadAttempts );
}

void BackupManager::uploadBackup( int attempts ) {
    auto retry = [this, attempts]() {
        auto newAttempts = attempts - 1;
        if( newAttempts <= 0 ) {
            m_error = "Failed to upload backup archive";
            LOGGER_ERROR_RAW( cc::error( m_error ) );
            getMessenger().sendProgressError( BACKUP_PROGRESS, Errors::CANT_UPLOAD_DATA, m_error );
            m_pendingOperationId = 0;
            clearBackupDirectory();
            return;
        }
        auto task = m_taskLoop.createPostponedTask(
            [this, newAttempts]( Services::PostponedTask& ) { uploadBackup( newAttempts ); } );
        task->start( m_delayOnFailedLoad, false );
    };

    LOGGER_INFO_RAW( cc::info( "Trying to upload backup" ) );
    if( !m_uploadFile( m_backupArchivePath,
                       m_backupMeta,
                       [this, retry]( const Json& userData,
                                      const std::string& error,
                                      const Common::CloudStorageInterface::FileKey& fileKey ) {
                           m_taskLoop.addTask( [this, retry, error]() {
                               if( error.empty() ) {
                                   m_pendingOperationId = 0;
                                   clearBackupDirectory();
                                   LOGGER_INFO_RAW( cc::info( "Uploaded backup archive" ) );
                                   getMessenger().sendProgressStatus( BACKUP_PROGRESS, "finished", 100 );
                               }
                               else {
                                   LOGGER_ERROR_RAW( cc::error( "Error uploading backup: " ) + cc::note( error ) );
                                   retry();
                               }
                           } );
                       } ) ) {
        LOGGER_ERROR_RAW( cc::error( "Error initializing backup upload" ) );
        retry();
    }
}

void BackupManager::onBackupServiceRestartedDuringLock( const std::string& service ) {
    m_error = "Firmware service " + service + " restarted during creating backup";
    LOGGER_ERROR_RAW( cc::error( m_error ) );
    // Wait for onUnlocked for reporting this error
}

// Restore stages progress distribution
//  download -  20% (20%)
//  lock     -  30% (10%)
//  recovery -  60% (30%)
//  restore  - 100% (40%)
void BackupManager::restore( const Common::CloudStorageInterface::FileKey& key ) {
    LOGGER_INFO_RAW( cc::info( "Prepare for restoring backup" ) );

    if( m_pendingOperationId || m_backupLockManager.isLocked() || m_restoreLockManager.isLocked() ) {
        LOGGER_ERROR_RAW( cc::error( "Other operation in progress" ) );
        return;
    }

    m_pendingOperationId = ++m_operationIdCounter;
    m_error = "";
    m_intermediateRestoreDirectory = getBackupDirectory() + "/" + Resources::DownloadedSubdirectory;
    m_restoreArchivePath = m_intermediateRestoreDirectory + ".tgz";

    LOGGER_INFO_RAW( cc::info( "Starting backup archive download" ) );
    getMessenger().sendProgressStatus( RESTORE_PROGRESS, "started", 0 );

    m_backupKey = key;
    downloadBackup( DownloadAttempts );
}

void BackupManager::downloadBackup( int attempts ) {
    auto retry = [this, attempts]() {
        auto newAttempts = attempts - 1;
        if( newAttempts <= 0 ) {
            m_error = "Failed to download backup archive";
            LOGGER_ERROR_RAW( cc::error( m_error ) );
            getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::CANT_DOWNLOAD_DATA, m_error );
            m_pendingOperationId = 0;
            clearBackupDirectory();
            return;
        }
        auto task = m_taskLoop.createPostponedTask(
            [this, newAttempts]( Services::PostponedTask& ) { downloadBackup( newAttempts ); } );
        task->start( m_delayOnFailedLoad, false );
    };

    clearBackupDirectory();
    LOGGER_INFO_RAW( cc::info( "Trying to download backup" ) );
    if( !m_downloadFile(
            m_backupKey, m_restoreArchivePath, [this, retry]( const Json& userData, const std::string& error ) {
                m_taskLoop.addTask( [this, retry, error]() {
                    if( error.empty() ) {
                        LOGGER_INFO_RAW( cc::info( "Downloaded backup archive" ) );
                        if( !m_backupKey.md5Checksum.empty() ) {
                            auto isError = checkAndCalculateMD5Checksum();
                            if( !isError ) {
                                m_error = "MD5 Checksum of downloaded file doesn't match";
                                LOGGER_ERROR_RAW( cc::error( m_error ) );
                                getMessenger().sendProgressError(
                                    RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
                                return;
                            }
                        }
                        startRestore();
                    }
                    else {
                        LOGGER_ERROR_RAW( cc::error( "Error downloading backup: " ) + cc::note( error ) );
                        retry();
                    }
                } );
            } ) ) {
        LOGGER_ERROR_RAW( cc::error( "Error initializing backup download" ) );
        retry();
    }
}

bool BackupManager::parseEzloRestoreConfig() {
    if( !ezlo::common::APIs::file2json( ( m_intermediateRestoreDirectory + "/config.json" ).c_str(),
                                        m_restoreConfig ) ) {
        m_error = "Failed to get backup config";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_pendingOperationId = 0;
        clearBackupDirectory();
        return false;
    }

    auto type = json_get_str( m_restoreConfig, "type" );
    if( type != Resources::BackupTypeNative ) {
        m_error = "Unsupported backup type, " + type + " instead of " + Resources::BackupTypeNative;
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_pendingOperationId = 0;
        clearBackupDirectory();
        return false;
    }

    auto version = json_get_str( m_restoreConfig, "version" );
    if( version != Resources::BackupVersion ) {
        m_error = "Unsupported backup version, " + version + " instead of " + Resources::BackupVersion;
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_pendingOperationId = 0;
        clearBackupDirectory();
        return false;
    }
    return true;
}

bool BackupManager::isVeraBackup() {
    return filesystem::isExist( m_intermediateRestoreDirectory + "/etc/mios_backup.info" );
}

bool BackupManager::checkAndCalculateMD5Checksum() {
    std::ifstream file( m_restoreArchivePath, std::ios::binary );
    if( !file ) {
        LOGGER_ERROR_RAW( cc::error( "Downloaded file not found: " + m_restoreArchivePath ) );
    }
    else {
        auto calculatedMD5Checksum = CryptoUtils::Digests::md5( file );

        if( m_backupKey.md5Checksum != calculatedMD5Checksum ) {
            LOGGER_ERROR_RAW(
                cc::error( "downloaded file md5Checksum is not correct, file: " + m_restoreArchivePath ) );
            LOGGER_DEBUG_RAW( cc::debug( "Requested checksum: " + m_backupKey.md5Checksum +
                                         " not equal to Calculated checksum: " + calculatedMD5Checksum ) );
            return false;
        }
    }
    return true;
}
void BackupManager::startRestore() {
    getMessenger().sendProgressStatus( RESTORE_PROGRESS, "process", 20 );

    if( !filesystem::createFolder( m_intermediateRestoreDirectory ) ||
        !filesystem::clearFolder( m_intermediateRestoreDirectory ) ) {
        m_error = "Failed to create intermediate restore directory";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_pendingOperationId = 0;
        clearBackupDirectory();
        return;
    }

    const std::string command = "cd " + m_intermediateRestoreDirectory + " && tar -xzf " + m_restoreArchivePath;
    if( std::system( command.c_str() ) != 0 ) {
        m_error = "Failed to extract data from backup archive";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_pendingOperationId = 0;
        clearBackupDirectory();
        return;
    }

    if( isVeraBackup() ) {
        m_restoreType = Resources::BackupTypeVera;
    }
    else {
        if( !parseEzloRestoreConfig() )
            return;
        m_restoreType = json_get_str( m_restoreConfig, "type" );
    }
    LOGGER_INFO_RAW( cc::info( "Restore type: " + m_restoreType ) );

    if( !m_restoreLockManager.lock() ) {
        m_error = "Failed to start locking firmware";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_pendingOperationId = 0;
        clearBackupDirectory();
        return;
    }

    // Waiting for onLocked method to be called
}

void BackupManager::onRestoreLocked( const std::string& error ) {
    if( !error.empty() ) {
        m_error = "Failed to lock firmware: " + error;
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        // Wait for onUnlocked for reporting this error
        return;
    }

    // NOTE: If some plugins are missing we should properly manually cleanup their data.
    //       E.g.: remove gateways or scene blocks. But we can not remove missing plugin's gateways
    //       during restore service launch as it does not have required environment initialized
    //       so fail restore in case if not all software is installed during restore that was used during making backup

    if( m_restoreType == Resources::BackupTypeNative && !isBackupHubStateCompatible() ) {
        m_error = "Backup software state is incompatible";
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        m_restoreLockManager.unlock();
        return;
    }

    m_intermediateBackupDirectory = generateBackupRecoveryDirectory();

    LOGGER_INFO_RAW( cc::info( "Start creating recovery backup" ) );
    getMessenger().sendProgressStatus( RESTORE_PROGRESS, "process", 30 );
    prepareCreateBackup( m_restoreLockManager,
                         std::bind( &BackupManager::handleRestoreResponse,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3 ) );
}

void BackupManager::handleRestoreResponse( const std::string& service, int operationId, const JsonObject& response ) {
    if( m_pendingOperationId != operationId ) {
        LOGGER_DEBUG_RAW( cc::debug( "Got outdated response on recovery backup request from " ) + cc::note( service ) +
                          cc::debug( " service" ) );
        // Ignore this response, looks like we've canceled this backup already
        return;
    }

    if( hasError( response ) || !json_get_bool( response, "success", false ) ) {
        std::string message = json_get_str( response[ "error" ], "message" );
        std::string reason = json_get_str( response[ "error" ], "reason" );
        reason = message.empty() ? "Unknown reason" : message + ( reason.empty() ? "" : ": " + reason );
        LOGGER_ERROR_RAW( cc::error( "Service " ) + cc::note( service ) +
                          cc::error( " failed to create recovery backup: " ) + cc::note( reason ) );
        m_error = "Service " + service + " failed to create backup: " + reason;
        m_restoreLockManager.unlock();
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Service " ) + cc::note( service ) + cc::debug( " recovery backuped successfully" ) );
    m_backupedServices.insert( service );

    int leftServices = m_backupServices.size() - m_unavailableAddons - m_backupedServices.size();
    if( leftServices != 0 ) {
        LOGGER_TRACE_RAW( cc::trace( "Waiting for " ) + cc::num10( leftServices ) + cc::trace( " more response(s)" ) );
        return;
    }

    if( !m_error.empty() ) {
        return;
    }

    // All services are backuped
    LOGGER_DEBUG_RAW( cc::debug( "All services recovery backuped successfully" ) );
    getMessenger().sendProgressStatus( RESTORE_PROGRESS, "process", 60 );

    if( m_restoreType == Resources::BackupTypeVera ) {
        performVeraRestore();
        return;
    }
    else
        performEzloRestore();

    if( !m_error.empty() ) {
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "All services restored from backup successfully" ) );
    getMessenger().sendProgressStatus( RESTORE_PROGRESS, "finished", 100 );
}

void BackupManager::performEzloRestore() {
    LOGGER_INFO_RAW( cc::info( "Stopping firmware for backup restore" ) );
    // NOTE: assume we can not fail stopping services
    m_scriptManager.stopDaemons();
    m_restoreLockManager.assumeUnlocked();

    m_error = restoreData( m_intermediateRestoreDirectory );
    if( !m_error.empty() ) {
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        if( !performRecovery() ) {
            return;
        }
    }

    LOGGER_INFO_RAW( cc::info( "Starting firmware" ) );
    // NOTE: assume we can not fail starting daemons
    m_scriptManager.startDaemons();

    m_pendingOperationId = 0;
    clearBackupDirectory();
}

void BackupManager::performVeraRestore() {
    LOGGER_INFO_RAW( cc::info( "Stopping firmware for backup restore" ) );
    m_scriptManager.stopDaemons();
    m_restoreLockManager.assumeUnlocked();

    LOGGER_INFO_RAW( cc::info( "Start vera backup restore" ) );
    m_veraBackupManager.start( m_intermediateRestoreDirectory );
}

void BackupManager::onVeraRestoreFinished( std::string error ) {
    if( !error.empty() )
        m_error = error;

    m_scriptManager.stopDaemons();

    if( !m_error.empty() ) {
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        if( !performRecovery() ) {
            return;
        }
    }

    m_pendingOperationId = 0;
    clearBackupDirectory();

    LOGGER_INFO_RAW( cc::info( "Starting firmware" ) );
    m_scriptManager.startDaemons();

    if( !m_error.empty() ) {
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "All services restored from backup successfully" ) );
    getMessenger().sendProgressStatus( RESTORE_PROGRESS, "finished", 100 );
}

bool BackupManager::isBackupHubStateCompatible() {
    auto currentHubState = m_restoreLockManager.getHubState();
    std::set< std::string > availableAddons;
    for( auto& addon : currentHubState.addons ) {
        auto addonService = "addon." + addon.id;
        availableAddons.insert( addon.id );
    }
    std::set< std::string > availablePlugins;
    for( auto& plugin : currentHubState.plugins ) {
        availablePlugins.insert( plugin.id );
    }

    auto backupHubState = CommonUtils::HubState::fromJson( m_restoreConfig[ "software" ] );
    for( auto& addon : backupHubState.addons ) {
        if( availableAddons.count( addon.id ) == 0 ) {
            LOGGER_ERROR_RAW( cc::error( "Addon " ) + cc::note( addon.id ) + cc::error( " is not available" ) );
            return false;
        }
    }

    for( auto& plugin : backupHubState.plugins ) {
        if( availablePlugins.count( plugin.id ) == 0 ) {
            // Following condition is ugly workaround because the fast fix is needed for Behome.
            // It is needed to find a better solution for cases when the plugin is removed.
            if( plugin.id == "zwave_ccellular" ) {
                continue;
            }
            LOGGER_ERROR_RAW( cc::error( "Plugin " ) + cc::note( plugin.id ) + cc::error( " is not available" ) );
            return false;
        }
    }

    return true;
}

std::string BackupManager::restoreData( const std::string& intermediateBackupDirectory ) {
    LOGGER_INFO_RAW( cc::info( "Resetting data" ) );
    for( auto& collectionName : m_dbClient.getAllNames() ) {
        LOGGER_TRACE_RAW( cc::trace( "Resetting table: " ) + cc::note( collectionName ) );
        m_dbClient.drop( collectionName );
    }

    auto backupHubState = CommonUtils::HubState::fromJson( m_restoreConfig[ "software" ] );
    std::set< std::string > availableAddons;
    for( auto& addon : backupHubState.addons ) {
        auto addonService = "addon." + addon.id;
        availableAddons.insert( addonService );
    }

    // TODO: run services in parallel
    for( const auto& servicePair : m_backupServices ) {
        auto& service = servicePair.first;
        auto& binary = servicePair.second;

        if( service.find( "addon." ) == 0U && availableAddons.find( service ) == availableAddons.end() ) {
            LOGGER_WARN_RAW( cc::warn( "Addon " ) + cc::note( service ) + cc::warn( " is not available" ) );
            continue;
        }

        LOGGER_DEBUG_RAW( cc::debug( "Running backup restore for " ) + cc::note( service ) + cc::debug( " service" ) );

        int result = m_scriptManager.restoreBackupCommand( binary, intermediateBackupDirectory );
        if( result != 0 ) {
            std::string reason = "unknown reason";
            if( WIFEXITED( result ) ) {
                reason = CommonUtils::RestoreHelper::returnCodeToString(
                    static_cast< CommonUtils::RestoreHelper::ReturnCode >( WEXITSTATUS( result ) ) );
            }
            else if( WIFSIGNALED( result ) ) {
                reason = "Got signal " + std::to_string( WTERMSIG( result ) );
            }
            return "Service " + service + " failed to restore backup: " + reason;
        }
    }

    LOGGER_INFO_RAW( cc::info( "Migrating data" ) );
    if( !m_scriptManager.migrateData() ) {
        return "Failed to migrate data";
    }
    return "";
}

bool BackupManager::performRecovery() {
    LOGGER_INFO_RAW( cc::info( "Performing recovery" ) );
    auto error = restoreData( generateBackupRecoveryDirectory() );

    if( !error.empty() ) {
        LOGGER_ERROR_RAW( cc::error( m_error ) );
        LOGGER_INFO_RAW( cc::info( "Resetting data and recovery" ) );
        clearBackupDirectory();
        m_scriptManager.resetData();
        return false;
    }
    return true;
}

void BackupManager::onRestoreUnlocked() {
    if( !m_error.empty() ) {
        m_pendingOperationId = 0;
        clearBackupDirectory();
        getMessenger().sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, m_error );
        m_error = "";
        return;
    }
}

void BackupManager::onRestoreServiceRestartedDuringLock( const std::string& service ) {
    m_error = "Firmware service " + service + " restarted during restoring backup";
    LOGGER_ERROR_RAW( cc::error( m_error ) );
    // Wait for onRestoreUnlocked for reporting this error
}

const std::string& BackupManager::getBackupDirectory() const {
    return m_backupDirectory;
}

std::string BackupManager::generateBackupRecoveryDirectory() const {
    return getBackupDirectory() + "/" + Resources::RecoverySubdirectory;
}

void BackupManager::clearBackupDirectory() {
    if( !filesystem::clearFolder( getBackupDirectory() ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to clear backup directory: " ) + cc::p( m_backupDirectory ) );
    }
}

std::string BackupManager::getDatabaseVersion() {
    auto result = m_dbClient.findOne( DbVersionsDBName, JSON( db::ID_FIELD_NAME << "firmware" ), nullptr );
    if( result.empty() ) {
        return "";
    }
    auto version = json_get_str( result, "version" );
    return version;
}

int BackupManager::getNumberOfDevices() {
    return (int) m_dbClient.count( HaDevicesDBName, JSON_EMPTY_OBJ );
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
