#include "backupd/backup_service.h"
#include "backupd/script_events/backup_creating_started.h"
#include "backupd/script_events/backup_creating_finished.h"
#include "json_utils/json_utils.h"
#include "backupd/resources/backup_resources.h"
#include "config_reader/config_keys.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/cpp14.h"
#include "cloud_storage/cloud_storage_utils.h"
#include "base_utils/timeutils.h"
#include "cored/house_modes/house_mode_utils.h"

namespace HubPlatform {
namespace BackupUpdateManager {

Service::Service()
: Services::Service( Resources::ServiceName ) {}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case BACKUP_CREATE:
        addTask( [this, request]() { m_backupManager->create(); } );
        return JSON_EMPTY_OBJ;
    case BACKUP_RESTORE: {
        Common::CloudStorageInterface::FileKey fileKey;
        fileKey.key = json_get_str( request, "key" );
        fileKey.uuid = json_get_str( request, "uuid" );
        fileKey.md5Checksum = json_get_str( request, "checksum" );
        addTask( [this, fileKey]() { m_backupManager->restore( fileKey ); } );
        return JSON_EMPTY_OBJ;
    }
    default:
        return Services::Service::handleRequestFromBus( request, requestType );
    }
}

bool Service::handleAsyncRequestFromBus( const JsonObject& request,
                                         MessageSubClasses requestType,
                                         const std::string& uuid,
                                         const std::string& sender ) {
    switch( requestType ) {
    case BACKUP_LIST: {
        Common::CloudStorageInterface::getFilesList(
            *this,
            Common::CloudStorageInterface::StorageType::ArchiveControllerBackup,
            [this, sender, uuid]( const Json& response ) {
                //  response = {"list": [{
                //      "bucket": "default",
                //      "file_format": "tgz",
                //      "filename": "backup_all_1611501943068680376",
                //      "key": "6Q3ddba...B6vSvxutVC",
                //      "location_uuid": "af8bb170-...-17e2d5947cc1",
                //      "size": "1575",
                //      "timestamp": "1611501944340",
                //      "type": "archive_controller_backup",
                //      "uuid": "6cfa2d40-...-b5665010194e"
                //  },...]}
                JsonArrayBuilder list;
                JsonArray filesList;
                // TODO: properly handle errors
                if( json_get_arr( response, "list", filesList ) ) {
                    if( filesList.is_array() ) {
                        for( const JsonObject& item : filesList )
                            if( item.is_object() ) {
                                auto itemUuid = json_get_str( item, "uuid" );
                                auto itemKey = json_get_str( item, "key" );
                                auto md5Checksum = json_get_str( item, "md5_checksum" );
                                auto bucketType = json_get_str( item, "bucket" );
                                // TODO: get initial timestamp instead of cloud timestamp
                                auto itemTimestamp = json_get_str( item, "timestamp" );
                                if( !itemUuid.empty() && !itemKey.empty() && !itemTimestamp.empty() &&
                                    !md5Checksum.empty() ) {
                                    list << JSON( "uuid" << itemUuid << "key" << itemKey << "md5_checksum"
                                                         << md5Checksum << "timestamp"
                                                         << std::strtoll( itemTimestamp.c_str(), nullptr, 10 )
                                                         << "bucket" << bucketType );
                                }
                                else if( !itemUuid.empty() && !itemKey.empty() && !itemTimestamp.empty() ) {
                                    list << JSON( "uuid" << itemUuid << "key" << itemKey << "timestamp"
                                                         << std::strtoll( itemTimestamp.c_str(), nullptr, 10 )
                                                         << "bucket" << bucketType );
                                }
                            }
                    }
                }
                sendAsyncResponse( sender, JSON( "backups" << list ), uuid );
            },
            std::chrono::seconds( 30 ) );
        return true;
    }
    default:
        return false;
    }
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        if( message.msgSubClass == SERVICE_STARTED ) {
            const std::string serviceId = json_get_str( message.data, "id" );
            if( m_backupLockManager ) {
                m_backupLockManager->onServiceStarted( serviceId );
            }
            if( m_restoreLockManager ) {
                m_restoreLockManager->onServiceStarted( serviceId );
            }
        }
        break;

    case SYSTEM_INFO:
        if( message.group == GroupsNames::BROADCAST_GROUP && message.msgSubClass == SYSTEM_BROADCAST ) {
            handleInfoCmdFromBus( message.data );
        }
        break;

    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Service::handleInfoCmdFromBus( const Json& command ) {
    auto type = json_get_str( command, "type" );

    if( type == "hub.backup.create" ) {
        m_backupManager->create();
    }
    else if( type.find( "hub.backup" ) == 0 ) {
        Log::error( cc::error( "Got unknown request " ) + cc::note( type ) );
    }
}

void Service::setUp() {
    Services::Service::setUp();

    auto uploadBackupFile =
        [this]( const std::string& file, const Json& meta, BackupManager::UploadResultCallback response ) {
            Common::CloudStorageInterface::UploadFileData uploadData( file );
            uploadData.metadata = meta;
            return ( m_cloudStorageInterface->uploadBackup(
                         uploadData,
                         Common::CloudStorageInterface::StorageType::ArchiveControllerBackup,
                         std::move( response ) ) == Common::CloudStorageInterface::Errors::Ok );
        };

    auto downloadFile = [this]( const Common::CloudStorageInterface::FileKey& fileKey,
                                const std::string& file,
                                BackupManager::DownloadResultCallback response ) {
        return ( m_cloudStorageInterface->downloadFile( fileKey, file, std::move( response ) ) ==
                 Common::CloudStorageInterface::Errors::Ok );
    };

    m_cloudStorageInterface = std::make_unique< Common::CloudStorageInterface::RemoteInterface >( getDBClient() );
    m_scriptManager = std::make_unique< ScriptManager >();
    m_backupLockManager = std::make_unique< GlobalLockManager >( *this, "backup" );
    m_restoreLockManager = std::make_unique< GlobalLockManager >( *this, "restore" );
    m_veraBackupManager = std::make_unique< VeraBackupManager >( *this, *m_scriptManager );
    m_backupManager = std::make_unique< BackupManager >( *m_backupLockManager,
                                                         *m_restoreLockManager,
                                                         *this,
                                                         *m_scriptManager,
                                                         *m_veraBackupManager,
                                                         getDBClient(),
                                                         *this,
                                                         uploadBackupFile,
                                                         downloadFile );
    m_configReader = std::make_unique< ConfigReader >();
    m_dailyBackupTimer = std::make_unique< NativeTimer >( [this]() { startDailyBackup(); } );

    m_backupManager->setUp();

    int backupOffset = 600;
    if( m_configReader->hasValue( ConfigKeys::backupOffsetTime ) ) {
        backupOffset = m_configReader->getInt( ConfigKeys::backupOffsetTime );
    }
    m_dailyBackupScheduler =
        std::make_unique< DailyBackupScheduler >( getDBClient(), *m_dailyBackupTimer, backupOffset );

    try {
        int timeoutSec = 180;
        if( m_configReader->hasValue( ConfigKeys::scriptRunTimeout ) ) {
            timeoutSec = m_configReader->getInt( ConfigKeys::scriptRunTimeout );
        }
        LOGGER_INFO( "timeout: " + std::to_string( timeoutSec ) );
        m_scriptManager->setScriptRunTimeout( std::chrono::seconds( timeoutSec ) );
        const char* isComponentTestRunning = std::getenv( "RUNNING_COMPONENT_TESTS" );
        if( !isComponentTestRunning ) {
            m_dailyBackupScheduler->setUp();
        }
    }
    catch( std::exception& e ) {
        LOGGER_ERROR( "setting timeout error: " + e.what() );
    }
}

void Service::sendProgressStatus( MessageSubClasses processType, std::string status, int percentage ) {
    JsonObject data;
    json_add_null( data, "error" );
    json_add_str( data, "serial", getSerial() );

    json_add_int( data, "completed", percentage );
    json_add_str( data, "status", status );

    Services::Message message = { GroupsNames::BACKUPD_BROADCASTS, EVENT, processType, data };
    send( message );

    if( processType == BACKUP_PROGRESS ) {
        if( status == "started" ) {
            emit( BackupCreatingStarted() );
        }
        else if( status == "finished" ) {
            emit( BackupCreatingFinished( true ) );
        }
    }
}

void Service::sendProgressError( MessageSubClasses processType, Errors::Error error, const std::string& reason ) {
    JsonObject data;
    setError( data, error, reason );
    json_add_str( data, "serial", getSerial() );

    json_add_int( data, "completed", 100 );
    json_add_str( data, "status", "aborted" );

    Services::Message message = { GroupsNames::BACKUPD_BROADCASTS, EVENT, processType, data };
    send( message );

    if( processType == BACKUP_PROGRESS ) {
        emit( BackupCreatingFinished( false ) );
    }
}

void Service::sendBackupRequest( const std::string& serviceName,
                                 const std::string& intermediateBackupDirectory,
                                 const Json& config,
                                 std::function< void( const JsonObject& message ) > responseCallback ) {
    auto message = JSON( "directory" << intermediateBackupDirectory << "config" << config );
    sendAsyncRequest( serviceName, message, BACKUP_CREATE, std::move( responseCallback ), std::chrono::minutes( 1 ) );
}

void Service::sendVeraBackupRequest( const std::string& serviceName,
                                     const std::string& intermediateBackupDirectory,
                                     const std::string& action,
                                     std::function< void( const JsonObject& message ) > responseCallback ) {
    auto message = JSON( "directory" << intermediateBackupDirectory << "action" << action );
    sendAsyncRequest(
        serviceName, message, BACKUP_RESTORE_VERA, std::move( responseCallback ), std::chrono::minutes( 5 ) );
}

void Service::startDailyBackup() {
    addTask( [this]() {
        std::string phase;
        std::string status;
        std::tie( phase, status ) = CoreDHouseModes::getAlarmPhase( getDBClient() );
        if( m_dailyBackupTimer->isRunning() && phase == "idle" ) {
            m_dailyBackupScheduler->setPreviousBackupRunTime( std::chrono::system_clock::now() );
            m_backupManager->create();
        }
    } );
}

const std::string& Service::getSerial() {
    if( m_serial.empty() ) {
        m_serial = HubInfoHelper::getSerial();
    }
    return m_serial;
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
