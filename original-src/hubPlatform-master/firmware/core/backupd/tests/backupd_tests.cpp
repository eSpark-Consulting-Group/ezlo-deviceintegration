#include "service_interfaces/tests/fixtures/fake_service_fixture.h"
#include "test_utils/temporary_root_folder_wrapper.h"
#include "system_utils/filesystem.h"
#include "system_utils/environment.h"

#include "backupd/backup/backup_script_manager.h"
#include "backupd/backup/backup_manager.h"
#include "backupd/backup_messenger.h"
#include "backupd/backup/vera_backup_manager.h"

#include "backupd/tests/test_files.h"
#include "crypto_utils/digests.h"
#include "base_utils/native_timer.h"
#include "backupd/backup/daily_backup_schedule.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>

/*

TESTPLAN

DONE    1. Create backup
DONE        1.1 Successful
DONE        1.2 Failed to start locking firmware
DONE        1.3 Failed to locking firmware
DONE        1.4 Failed service backup
DONE        1.5 One failed upload attempt
DONE        1.6 Failed upload
DONE    2. Restore backup
DONE        2.1 Successful
DONE        2.2 One failed download attempt
DONE        2.3 Failed download
DONE        2.4 Failed to start locking firmware
DONE        2.5 Failed to locking firmware
DONE        2.6 Failed recovery backup
DONE        2.7 Failed service restore
DONE        2.8 Failed data migration
DONE        2.9 Failed service restore and recovery
DONE        2.9.1 Failed download on checking MD5 checksum
DONE        2.9.2 Successful download on passing MD5 checksum
DONE    3. Restore vera backup
DONE        3.1 Successful
DONE        3.2 Successful with extended time
DONE        3.3 Service failed
DONE    4. Backup Scheduler
DONE        4.1 Successful
DONE        4.2 add backup schedule
DONE        4.3 add previous backup runtime
DONE        4.4 update previous backup runtime
DONE        4.5 backup offset delay
DONE    5. Backup Metadata new fields
DONE        5.1 checking database_version and number of devices fields in meta data of backup
*/

namespace HubPlatform {
namespace UT {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

using namespace BackupUpdateManager;

class MockMessenger : public Messenger {
public:
    MOCK_METHOD3( sendProgressStatus, void( MessageSubClasses processType, std::string status, int percentage ) );
    MOCK_METHOD3( sendProgressError,
                  void( MessageSubClasses processType, Errors::Error error, const std::string& reason ) );
    MOCK_METHOD4( sendBackupRequest,
                  void( const std::string& serviceName,
                        const std::string& intermediateBackupDirectory,
                        const Json& config,
                        std::function< void( const JsonObject& message ) > responseCallback ) );
    MOCK_METHOD4( sendVeraBackupRequest,
                  void( const std::string& serviceName,
                        const std::string& intermediateBackupDirectory,
                        const std::string& action,
                        std::function< void( const JsonObject& message ) > responseCallback ) );
};

class MockScriptManager : public ScriptManager {
public:
    MOCK_METHOD2( startDaemon, void( const std::string daemonName, const std::vector< std::string > args ) );
    MOCK_METHOD0( startDaemons, void() );
    MOCK_METHOD0( stopDaemons, void() );
    MOCK_METHOD2( restoreBackupCommand, int( const std::string& binaryName, const std::string& targetDirectory ) );

    MOCK_METHOD0( migrateData, bool() );
    MOCK_METHOD0( resetData, void() );
};

class FakeGlobalLockManager : public IGlobalLockManager {
public:
    enum class Error { None, FailStartLocking, FailLocking };

    FakeGlobalLockManager( Services::TaskLoop& taskLoop, Error error = Error::None )
    : m_taskLoop( taskLoop )
    , m_error( error ) {
        m_state.addons.push_back( CommonUtils::AddonSummary{ "zwave", HubPlatform::Common::BuildVersion( 1, 0 ) } );
    }
    bool start( ResultCallback lockedCallback,
                Callback unlockedCallback,
                ServiceCallback serviceRestartedWhileLockedCallback ) override {
        m_lockedCallback = std::move( lockedCallback );
        m_unlockedCallback = std::move( unlockedCallback );
        m_serviceRestartedWhileLockedCallback = std::move( serviceRestartedWhileLockedCallback );
        return true;
    }

    bool lock() override {
        if( m_error == Error::FailStartLocking ) {
            return false;
        }
        if( m_error == Error::FailLocking ) {
            m_taskLoop.addTask( [this]() {
                m_lockedCallback( "Some reason" );
                m_taskLoop.addTask( [this]() { m_unlockedCallback(); } );
            } );
            return true;
        }
        m_lockedCallback( "" );
        return true;
    }

    bool unlock() override {
        m_unlockedCallback();
        return true;
    }

    bool assumeUnlocked() override {
        return true;
    }

    bool isLocked() const override {
        return false;
    }

    CommonUtils::HubState getHubState() const override {
        return m_state;
    }

private:
    Services::TaskLoop& m_taskLoop;
    CommonUtils::HubState m_state;

    ResultCallback m_lockedCallback;
    Callback m_unlockedCallback;
    ServiceCallback m_serviceRestartedWhileLockedCallback;
    Error m_error;
};

class FakeVeraBackupManager : public IVeraBackupManager {
public:
    void setUp( OnFinishedCallback onFinished ){};
    void start( std::string backupDirectory ){};

protected:
    void wait( std::chrono::milliseconds time ){};
};

class BackupdManagerTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        m_dbClient = createDbClient();
        m_dbClient->clean();
        Log::setLocalLogLevel( Log::DEBUG );

        m_future = m_helper.runInThread( m_taskLoop );
    }

    void TearDown() override {
        m_dbClient->clean();
        m_dbClient.reset();
        m_taskLoop.addTask( [this]() { m_taskLoop.stop(); } );
        m_future.get();
        m_helper.clearLoop( m_taskLoop );
    }

    IDbClient& getDBClient() {
        assert( m_dbClient );
        return *m_dbClient;
    }

    void waitForTasks() {
        constexpr int Indirections = 4;
        for( int i = 0; i < Indirections; ++i ) {
            m_helper.waitForTasks( m_taskLoop );
        }
    }

    Services::TaskLoop& getTaskLoop() {
        return m_taskLoop;
    }

    BackupManager::UploadFileCallback uploadFile( int failedAttempts = 0 ) {
        int attempts = 0;
        return [attempts, failedAttempts, this](
                   const std::string& file, const Json& meta, BackupManager::UploadResultCallback response ) mutable {
            std::ifstream fileChecksum( file, std::ios::binary );
            m_checksum = CryptoUtils::Digests::md5( fileChecksum );
            m_metaData = meta;
            attempts++;
            if( attempts <= failedAttempts ) {
                return false;
            }
            else {
                filesystem::copyFile( file, HubPlatform::Environment::getTempDataFolder() + "/uploaded.tgz" );
                response( {}, "", { "test_key", "test_uuid", "" } );
                return true;
            }
        };
    }

    BackupManager::DownloadFileCallback downloadFile( int failedAttempts = 0 ) {
        int attempts = 0;
        return [attempts, failedAttempts]( const Common::CloudStorageInterface::FileKey& fileKey,
                                           const std::string& path,
                                           BackupManager::DownloadResultCallback response ) mutable {
            attempts++;
            if( attempts <= failedAttempts ) {
                return false;
            }
            else {
                filesystem::copyFile( HubPlatform::Environment::getTempDataFolder() + "/uploaded.tgz", path );
                response( {}, "" );
                return true;
            }
        };
    }

    BackupManager::DownloadFileCallback downloadVeraFile( int failedAttempts = 0 ) {
        int attempts = 0;
        return [attempts, failedAttempts]( const Common::CloudStorageInterface::FileKey& fileKey,
                                           const std::string& path,
                                           BackupManager::DownloadResultCallback response ) mutable {
            attempts++;
            if( attempts <= failedAttempts ) {
                return false;
            }
            else {
                filesystem::copyFile( HubPlatform::PathVeraBackupFile, path );
                response( {}, "" );
                return true;
            }
        };
    }

    void successBackupRequest( const std::string& serviceName,
                               const std::string& intermediateBackupDirectory,
                               const Json& config,
                               std::function< void( const JsonObject& message ) > responseCallback ) {
        addTask( [=]() { responseCallback( JSON( "success" << true ) ); } );
    }

    void failBackupRequest( const std::string& serviceName,
                            const std::string& intermediateBackupDirectory,
                            const Json& config,
                            std::function< void( const JsonObject& message ) > responseCallback ) {
        addTask( [=]() { responseCallback( JSON( "success" << false ) ); } );
    }

    void successVeraBackupRequest( const std::string& serviceName,
                                   const std::string& intermediateBackupDirectory,
                                   const std::string& action,
                                   std::function< void( const JsonObject& message ) > responseCallback ) {
        addTask( [=]() {
            responseCallback( JSON( "status"
                                    << "finished" ) );
        } );
    }

    void inProgressVeraBackupRequest( const std::string& serviceName,
                                      const std::string& intermediateBackupDirectory,
                                      const std::string& action,
                                      std::function< void( const JsonObject& message ) > responseCallback ) {
        addTask( [=]() {
            responseCallback( JSON( "status"
                                    << "in_progress" ) );
        } );
    }

    void failVeraBackupRequest( const std::string& serviceName,
                                const std::string& intermediateBackupDirectory,
                                const std::string& action,
                                std::function< void( const JsonObject& message ) > responseCallback ) {
        addTask( [=]() {
            responseCallback( JSON( "status"
                                    << "failed" ) );
        } );
    }

    template < typename Callback, decltype( std::declval< Callback >()() )* = nullptr >
    void addTask( Callback&& f, bool isPeriodic = false ) {
        m_taskLoop.template addTask( f, isPeriodic );
    }

    void setVeraServiceStartupDelay( VeraBackupManager& manager, std::chrono::milliseconds delay ) {
        manager.m_daemonsStartupDelay = delay;
    }

private:
    Services::TaskLoop m_taskLoop;
    Services::UT::TaskLoopTestHelper m_helper;
    std::future< void > m_future;

    Tests::TemporaryRootFolderWrapper m_rootFolderWrapper;
    std::unique_ptr< IDbClient > m_dbClient;

protected:
    std::string m_checksum;
    Json m_metaData;
};

TEST_F( BackupdManagerTestSuite, createBackupSuccessful /* 1.1 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendBackupRequest( "cored", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "extensions_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "external_controllers", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "hardware_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "network", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "scenes_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "ui_service", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "cloud_devices", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "addon.zwave", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 50 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 60 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "finished", 100 ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, createBackupFailedToStartLockingFirmware /* 1.2 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop(), FakeGlobalLockManager::Error::FailStartLocking );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL(
        messenger,
        sendProgressError( BACKUP_PROGRESS, Errors::BACKUP_CREATE_FAILED, "Failed to start locking firmware" ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, createBackupFailedToLockingFirmware /* 1.3 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop(), FakeGlobalLockManager::Error::FailLocking );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL(
        messenger,
        sendProgressError( BACKUP_PROGRESS, Errors::BACKUP_CREATE_FAILED, "Failed to lock firmware: Some reason" ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, createBackupFailedServiceBackup /* 1.4 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "network", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::failBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );

    EXPECT_CALL( messenger,
                 sendProgressError( BACKUP_PROGRESS,
                                    Errors::BACKUP_CREATE_FAILED,
                                    "Service network failed to create backup: Unknown reason" ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, createBackupFewFailedUploadAttempts /* 1.5 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile( 1 ),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 50 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 60 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "finished", 100 ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, createBackupFailedUpload /* 1.6 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile( 3 ),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 50 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 60 ) );
    EXPECT_CALL( messenger,
                 sendProgressError( BACKUP_PROGRESS, Errors::CANT_UPLOAD_DATA, "Failed to upload backup archive" ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();
}

#define EXPECT_SUCCESSFUL_BACKUP_CREATE()                                                  \
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );         \
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );        \
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )                              \
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) ); \
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 50 ) );        \
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 60 ) );        \
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "finished", 100 ) );

TEST_F( BackupdManagerTestSuite, restoreBackupSuccessful /* 2.1 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-cored", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-extensions_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-external_controllers", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-hardware_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-networkd", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-scenes_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-uid", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-cloud_devices", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "addons/ha-zwaved", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( true ) );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "finished", 100 ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupOneFailedDownloadAttempt /* 2.2 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile( 1 ),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-cored", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-extensions_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-external_controllers", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-hardware_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-networkd", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-scenes_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-uid", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-cloud_devices", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "addons/ha-zwaved", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( true ) );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "finished", 100 ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedDownload /* 2.3 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile( 3 ),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL(
        messenger,
        sendProgressError( RESTORE_PROGRESS, Errors::CANT_DOWNLOAD_DATA, "Failed to download backup archive" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedToStartLockingFirmware /* 2.4 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop(), FakeGlobalLockManager::Error::FailStartLocking );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL(
        messenger,
        sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, "Failed to start locking firmware" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedToLockingFirmware /* 2.5 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop(), FakeGlobalLockManager::Error::FailLocking );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL(
        messenger,
        sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, "Failed to lock firmware: Some reason" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedRecoveryBackup /* 2.6 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "network", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::failBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger,
                 sendProgressError( RESTORE_PROGRESS,
                                    Errors::BACKUP_RESTORE_FAILED,
                                    "Service network failed to create backup: Unknown reason" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedServiceRestore /* 2.7*/ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "addons/ha-zwaved", _ ) ).WillOnce( Return( 1 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( true ) );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger,
                 sendProgressError( RESTORE_PROGRESS,
                                    Errors::BACKUP_RESTORE_FAILED,
                                    "Service addon.zwave failed to restore backup: Got signal 1" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedDataMigration /* 2.8 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( false ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( true ) );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger,
                 sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, "Failed to migrate data" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedServiceRestoreAndRecovery /* 2.9 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "addons/ha-zwaved", _ ) ).WillOnce( Return( 1 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "addons/ha-zwaved", _ ) ).WillOnce( Return( 1 ) );
    EXPECT_CALL( scriptManager, resetData() );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupFailedOnMD5Check /* 2.9.1 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile( 1 ),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL(
        messenger,
        sendProgressError(
            RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, "MD5 Checksum of downloaded file doesn't match" ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "test_md5checksum" } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreBackupSuccessfulWithMD5 /* 2.9.2 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-cored", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-extensions_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-external_controllers", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-hardware_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-networkd", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-scenes_manager", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-uid", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "ha-cloud_devices", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, restoreBackupCommand( "addons/ha-zwaved", _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( true ) );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "finished", 100 ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", m_checksum } ); } );

    waitForTasks();
}

TEST_F( BackupdManagerTestSuite, restoreVeraBackupSuccessful /* 3.1 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    VeraBackupManager veraBackupManager( messenger, scriptManager );
    setVeraServiceStartupDelay( veraBackupManager, std::chrono::milliseconds( 0 ) );
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadVeraFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 1 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "addon.zwave", _, "migrate_devices", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 0 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "cored", _, "migrate_rooms", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 1 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "scenes_manager", _, "migrate_scenes", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "finished", 100 ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
    std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
}

TEST_F( BackupdManagerTestSuite, restoreVeraBackupSuccessfulWithExtendedTime /* 3.2 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    VeraBackupManager veraBackupManager( messenger, scriptManager );
    setVeraServiceStartupDelay( veraBackupManager, std::chrono::milliseconds( 0 ) );
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadVeraFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 1 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "addon.zwave", _, "migrate_devices", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::inProgressVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( messenger, sendVeraBackupRequest( "addon.zwave", _, "migrate_devices", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::inProgressVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( messenger, sendVeraBackupRequest( "addon.zwave", _, "migrate_devices", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 0 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "cored", _, "migrate_rooms", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 1 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "scenes_manager", _, "migrate_scenes", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "finished", 100 ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
    std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
}

TEST_F( BackupdManagerTestSuite, restoreVeraBackupWithFailure /* 3.3 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    VeraBackupManager veraBackupManager( messenger, scriptManager );
    setVeraServiceStartupDelay( veraBackupManager, std::chrono::milliseconds( 0 ) );
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadVeraFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_SUCCESSFUL_BACKUP_CREATE()

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 30 ) );
    EXPECT_CALL( messenger, sendBackupRequest( _, _, _, _ ) )
        .WillRepeatedly( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendProgressStatus( RESTORE_PROGRESS, "process", 60 ) );
    EXPECT_CALL( scriptManager, stopDaemons() );

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 1 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "addon.zwave", _, "migrate_devices", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 0 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "cored", _, "migrate_rooms", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, startDaemon( _, _ ) ).Times( AtLeast( 1 ) );
    EXPECT_CALL( messenger, sendVeraBackupRequest( "scenes_manager", _, "migrate_scenes", _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::failVeraBackupRequest ) );
    waitForTasks();

    EXPECT_CALL( scriptManager, stopDaemons() );
    EXPECT_CALL( scriptManager, restoreBackupCommand( _, _ ) ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( scriptManager, migrateData() ).WillOnce( Return( true ) );
    EXPECT_CALL( scriptManager, startDaemons() );
    EXPECT_CALL( messenger, sendProgressError( RESTORE_PROGRESS, Errors::BACKUP_RESTORE_FAILED, _ ) );

    addTask( [&]() { manager.restore( { "test_key", "test_uuid", "" } ); } );

    waitForTasks();
    std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
}

TEST_F( BackupdManagerTestSuite, backupScheduler /* 4.1 */ ) {
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendBackupRequest( "cored", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "extensions_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "external_controllers", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "hardware_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "network", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "scenes_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "ui_service", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "cloud_devices", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "addon.zwave", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 50 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 60 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "finished", 100 ) );

    NativeTimer timer( [&manager]() { manager.create(); } );
    BackupUpdateManager::DailyBackupScheduler backupScheduler( getDBClient(), timer, 600 );
    EXPECT_TRUE( backupScheduler.getBackupSchedule().count() == 0 );

    addTask( [&]() {
        manager.setUp();
        backupScheduler.setUp();
    } );

    waitForTasks();
    std::this_thread::sleep_for( std::chrono::seconds( 31 ) );
    EXPECT_FALSE( backupScheduler.getBackupSchedule().count() == 0 );
}

TEST_F( BackupdManagerTestSuite, addBackupScheduleToDatabase /*4.2*/ ) {
    NativeTimer timer( []() {} );
    BackupUpdateManager::DailyBackupScheduler backupScheduler( getDBClient(), timer, 600 );

    auto backupScheduleTime = backupScheduler.getBackupSchedule();
    EXPECT_TRUE( backupScheduleTime.count() == 0 );

    backupScheduler.setBackupSchedule( std::chrono::seconds( 2525 ) );
    backupScheduleTime = backupScheduler.getBackupSchedule();
    EXPECT_TRUE( backupScheduleTime.count() == 2525 );
}

TEST_F( BackupdManagerTestSuite, addPreviousBackupRunTimeToDatabase /*4.3*/ ) {
    NativeTimer timer( []() {} );
    BackupUpdateManager::DailyBackupScheduler backupScheduler( getDBClient(), timer, 600 );

    auto previousBackupRunTime = backupScheduler.getPreviousBackupRunTime();
    EXPECT_TRUE( previousBackupRunTime.count() == 0 );

    auto now = std::chrono::system_clock::now();
    auto nowSeconds = std::chrono::duration_cast< std::chrono::seconds >( now.time_since_epoch() );
    backupScheduler.setPreviousBackupRunTime( now );
    previousBackupRunTime = backupScheduler.getPreviousBackupRunTime();
    EXPECT_TRUE( previousBackupRunTime.count() == nowSeconds.count() );
}

TEST_F( BackupdManagerTestSuite, updatePreviousBackupRunTime /*4.4*/ ) {
    NativeTimer timer( []() {} );
    BackupUpdateManager::DailyBackupScheduler backupScheduler( getDBClient(), timer, 600 );

    auto now = std::chrono::system_clock::now();
    auto nowSeconds = std::chrono::duration_cast< std::chrono::seconds >( now.time_since_epoch() );
    backupScheduler.setPreviousBackupRunTime( now );
    auto previousBackupRunTime = backupScheduler.getPreviousBackupRunTime();
    EXPECT_TRUE( previousBackupRunTime.count() == nowSeconds.count() );

    auto clockNow = std::chrono::system_clock::now();
    auto clockNowSeconds = std::chrono::duration_cast< std::chrono::seconds >( clockNow.time_since_epoch() );
    backupScheduler.setPreviousBackupRunTime( clockNow );
    auto updatedBackupRunTime = backupScheduler.getPreviousBackupRunTime();
    EXPECT_TRUE( updatedBackupRunTime.count() == nowSeconds.count() );
}

TEST_F( BackupdManagerTestSuite, backupOffsetTime /*4.5*/ ) {
    int backupOffset = 600;
    NativeTimer timer( []() {} );
    BackupUpdateManager::DailyBackupScheduler backupScheduler( getDBClient(), timer, backupOffset );
    {
        auto expectedBackupOffsetTime = backupOffset - 30;
        auto backupOffsetTime = backupScheduler.processBackupOffsetTime( std::chrono::seconds( 30 ) );
        EXPECT_EQ( backupOffsetTime.count(), expectedBackupOffsetTime );
    }
    {
        auto expectedBackupOffsetTime = backupOffset - 60;
        auto backupOffsetTime = backupScheduler.processBackupOffsetTime( std::chrono::seconds( 60 ) );
        EXPECT_EQ( backupOffsetTime.count(), expectedBackupOffsetTime );
    }
    {
        auto expectedBackupOffsetTime = backupOffset - 90;
        auto backupOffsetTime = backupScheduler.processBackupOffsetTime( std::chrono::seconds( 90 ) );
        EXPECT_EQ( backupOffsetTime.count(), expectedBackupOffsetTime );
    }
    {
        auto expectedBackupOffsetTime = backupOffset - 300;
        auto backupOffsetTime = backupScheduler.processBackupOffsetTime( std::chrono::seconds( 300 ) );
        EXPECT_EQ( backupOffsetTime.count(), expectedBackupOffsetTime );
    }
}

TEST_F( BackupdManagerTestSuite, backupMetaDataFields /* 5.1 */ ) {
    getDBClient().insert( DbVersionsDBName,
                          JSON( "_id"
                                << "firmware"
                                << "version"
                                << "1.54" ) );
    getDBClient().insert( HaDevicesDBName,
                          JSON( "_id"
                                << "test_device1" ) );
    getDBClient().insert( HaDevicesDBName,
                          JSON( "_id"
                                << "test_device2" ) );
    InSequence s;
    FakeGlobalLockManager backupLockManager( getTaskLoop() );
    FakeGlobalLockManager restoreLockManager( getTaskLoop() );
    MockMessenger messenger;
    MockScriptManager scriptManager;
    FakeVeraBackupManager veraBackupManager;
    BackupManager manager( backupLockManager,
                           restoreLockManager,
                           messenger,
                           scriptManager,
                           veraBackupManager,
                           getDBClient(),
                           getTaskLoop(),
                           uploadFile(),
                           downloadFile(),
                           std::chrono::seconds( 0 ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "started", 0 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 20 ) );
    EXPECT_CALL( messenger, sendBackupRequest( "cored", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "extensions_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "external_controllers", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "hardware_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "network", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "scenes_manager", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "ui_service", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "cloud_devices", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );
    EXPECT_CALL( messenger, sendBackupRequest( "addon.zwave", _, _, _ ) )
        .WillOnce( Invoke( this, &BackupdManagerTestSuite::successBackupRequest ) );

    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 50 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "process", 60 ) );
    EXPECT_CALL( messenger, sendProgressStatus( BACKUP_PROGRESS, "finished", 100 ) );

    addTask( [&]() {
        manager.setUp();
        manager.create();
    } );

    waitForTasks();

    auto info = m_metaData[ "controller_info" ];
    ASSERT_TRUE( json_contains( info, "database_version" ) );
    ASSERT_TRUE( json_contains( info, "number_of_devices" ) );
    EXPECT_EQ( json_get_str( info, "database_version" ), "1.54" );
    EXPECT_EQ( json_get_int( info, "number_of_devices" ), 2 );
}

}  // namespace UT
}  // namespace HubPlatform
