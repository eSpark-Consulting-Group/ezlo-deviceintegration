#include <fstream>
#include "local_notifications.h"
#include "utils.h"
#include "exceptions.h"
#include "service_interfaces/service.h"
#include "log_utils/ezlo_log.h"
#include "base_utils/timeutils.h"
#include "jsonrpc/jsonrpc.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "resources.h"
#include "system_utils/system.h"
#include "base_utils/gregorian_calendar.h"
#include "base_utils/return_action.h"
#include "data_segments_deserializer.h"
#include "hub_settings/hub_settings_reader.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace CloudDevices {

const std::chrono::minutes LocalNotifications::LocalNotificationRequestPeriod = std::chrono::minutes( 60 );
const std::chrono::seconds LocalNotifications::CloudDataUpdateTaskPeriod = std::chrono::seconds( 1 );
const std::chrono::seconds LocalNotifications::MeshbotsTemplateUpdateTaskPeriod = std::chrono::seconds( 2 );
static constexpr int TcdsFileSizeLimit = 100;

LocalNotifications::LocalNotifications( Services::Service& service )
: m_service( service )
, m_retryCount( 0 )
, m_tcdsCurrentVersion( 0 )
, m_downloadAttempts( 0 )
, m_meshbotsCurrentVersion( 0 )
, m_fileSizeLimit( 0 )
, m_localNotificationDirectory( Environment::getDataFolder() + "/" + "local_notification" ) {
    filesystem::createFolder( m_localNotificationDirectory );
    Hub::SettingsReader reader( m_service.getDBClient() );
    auto fileSizeSetting = reader.findSetting( "tcds.file_size_limit" );
    if( fileSizeSetting ) {
        m_fileSizeLimit = fileSizeSetting->getIntValue();
    }
    else {
        m_fileSizeLimit = TcdsFileSizeLimit;
    }
    m_cloudDataHandler = std::make_unique< CloudDataHandler >();
    m_meshbotsTemplateHandler = std::make_unique< MeshbotsTemplateHandler >();
}

LocalNotifications::~LocalNotifications() {
    if( m_localNotificationTask ) {
        m_localNotificationTask->stop();
    }
    if( m_cloudDataUpdateTask ) {
        m_cloudDataUpdateTask->stop();
    }
    if( m_meshbotTemplateUpdateTask ) {
        m_meshbotTemplateUpdateTask->stop();
    }
    if( m_multipleVersionHandleTask ) {
        m_multipleVersionHandleTask->stop();
    }
}

void LocalNotifications::sendTcdsCurrentVersionRequest() {
    LOGGER_INFO( "Send request for TCDS and Meshbots Current Versions" );
    if( m_localNotificationTask && m_localNotificationTask->isActive() ) {
        m_localNotificationTask->stop();
    }

    m_isUpdate = false;
    initTcdsRecordDataToDb();
    m_localNotificationTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
        JsonObject localNotificationRequest = localNotificationRequestBuilder();
        m_service.sendAsyncRequest( GroupsNames::UI_GROUP,
                                    localNotificationRequest,
                                    CLOUD_MESSAGE,
                                    [this]( const JsonObject& message ) { handleTcdsRequestResponse( message ); },
                                    [this]() { handleTcdsRequestTimeout(); } );
    } );

    m_cloudDataUpdateTask =
        m_service.createPostponedTask( [this]( Services::PostponedTask& ) { startDownloadAndExtractTcds(); } );
    m_meshbotTemplateUpdateTask =
        m_service.createPostponedTask( [this]( Services::PostponedTask& ) { startDownloadAndExtractMeshbots(); } );

    m_multipleVersionHandleTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
        skippingToLatestCloudDataRequest();
        skippingToLatestMeshbotsTemplatesRequest();
    } );

    const char* isComponentTestRunning = std::getenv( "RUNNING_COMPONENT_TESTS" );
    if( !isComponentTestRunning ) {
        m_multipleVersionHandleTask->start( std::chrono::minutes( 1 ), true );
        m_localNotificationTask->start( std::chrono::seconds( 30 ), false );
    }
}

JsonObject LocalNotifications::localNotificationRequestBuilder() {
    using namespace LocalNotification::Resources;

    JsonObject message =
        JSON( Fields::Method << Methods::CloudTCDSCurrentVersionRequest << Fields::Api
                             << Methods::CloudTCDSCurrentVersionRequestApiVersion << Fields::Params << JSON_EMPTY_OBJ );
    return message;
}

void LocalNotifications::handleTcdsRequestResponse( const JsonObject& message ) {
    if( hasError( message ) ) {
        LOGGER_INFO_RAW( "Tcds Current version call has error: " + cc::j( message ) );
        JsonObject error;
        json_get_obj( message, "error", error );
        int errorCode = json_get_int( error, "code" );
        if( errorCode != 26 ) {  // FIXME: replace magic number to enum
            tcdsRequestTaskRestart();
        }
        // FIXME: Cleanup and return
    }

    LOGGER_DEBUG_RAW( "Tcds and meshbots Current versions received: " + cc::j( message ) );
    auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
    int currentVersion = json_get_int( message, "current_version" );
    if( currentVersion != m_tcdsCurrentVersion ) {
        if( m_cloudDataUpdateTask && m_cloudDataUpdateTask->isActive() ) {
            m_cloudDataUpdateTask->stop();
        }
        m_tcdsUrl = json_get_str( message, "url" );
        m_cloudS3Id = json_get_str( message, "s3_id" );
        if( !m_tcdsUrl.empty() ) {
            m_tcdsNextVersion = currentVersion;
            m_intermediateGenericDataDirectory = m_localNotificationDirectory + "/tcds_" + date;
            m_downloadedGenericDataArchivePath = m_intermediateGenericDataDirectory + ".tgz";
            m_cloudDataUpdateTask->start( CloudDataUpdateTaskPeriod, false );
        }
    }
    int meshbotsCurrentVersion = json_get_int( message, "meshbots_current_version" );
    if( meshbotsCurrentVersion != m_meshbotsCurrentVersion ) {
        if( m_meshbotTemplateUpdateTask && m_meshbotTemplateUpdateTask->isActive() ) {
            m_meshbotTemplateUpdateTask->stop();
        }
        m_meshbotsUrl = json_get_str( message, "meshbots_url" );
        m_meshbotS3Id = json_get_str( message, "meshbots_s3_id" );
        if( !m_meshbotsUrl.empty() ) {
            m_meshbotsNextVersion = meshbotsCurrentVersion;
            m_intermediateMeshbotsDirectory = m_localNotificationDirectory + "/meshbots_" + date;
            m_downloadedMeshbotsArchivePath = m_intermediateMeshbotsDirectory + ".tgz";
            m_meshbotTemplateUpdateTask->start( MeshbotsTemplateUpdateTaskPeriod, false );
        }
    }
    m_retryCount = -1;
    tcdsRequestTaskRestart();
}

void LocalNotifications::tcdsRequestTaskRestart() {
    auto random = rand() % 20;
    std::chrono::minutes randomInterval( random );

    if( m_retryCount < 5 ) {
        LOGGER_DEBUG_RAW(
            cc::debug( "starting local notification task again after interval of: " ) +
            cc::str( std::to_string( LocalNotificationRequestPeriod.count() + randomInterval.count() ) + " minutes" ) );
        m_localNotificationTask->start( LocalNotificationRequestPeriod + randomInterval, false );
        m_retryCount++;
    }
    else {
        m_localNotificationTask.reset();
        updateTcdsRecordInfo( Status::Unknown, "tcds request failed timeout reached" );
    }
}
void LocalNotifications::handleTcdsRequestTimeout() {
    LOGGER_DEBUG_RAW( "Tcds Current version timeout callback" );
    tcdsRequestTaskRestart();
}

void LocalNotifications::startDownloadAndExtractTcds() {
    if( m_isUpdate ) {
        sendProgressBroadcast( "started", 0, "cloudData" );
    }
    if( isStorageLimitReached() ) {
        LOGGER_ERROR_RAW( "TCDS file download failed, file storage limit exceed" );
        return;
    }
    LOGGER_DEBUG_RAW( "TCDS file Downloading start..." );
    updateTcdsRecordInfo( Status::Downloading );

    auto processSuccess = [this]() {
        m_service.createPostponedTask( [this]( Services::PostponedTask& ) { onSuccessFullDownloadTcds(); } )
            ->start( std::chrono::seconds( 0 ), false );
    };

    auto processFailure = [this]( const std::string& errorDescription ) {
        m_service
            .createPostponedTask( [this, errorDescription]( Services::PostponedTask& ) {
                onFailedToDownloadGenericData( errorDescription, "cloudData" );
            } )
            ->start( std::chrono::seconds( 0 ), false );
    };

    m_localNotificationDownloader.downloadFileAsync(
        m_tcdsUrl, m_downloadedGenericDataArchivePath, processSuccess, processFailure );
}

void LocalNotifications::startDownloadAndExtractMeshbots() {
    if( m_isUpdate ) {
        sendProgressBroadcast( "started", 0, "templates" );
    }
    if( isStorageLimitReached() ) {
        LOGGER_ERROR_RAW( "Meshbot file download failed, file storage limit exceed" );
        return;
    }
    LOGGER_DEBUG_RAW( "Meshbot file Downloading start..." );
    updateTcdsRecordInfo( Status::Downloading );

    auto processSuccess = [this]() {
        m_service.createPostponedTask( [this]( Services::PostponedTask& ) { onSuccessFullDownloadMeshbots(); } )
            ->start( std::chrono::seconds( 0 ), false );
    };

    auto processFailure = [this]( const std::string& errorDescription ) {
        m_service
            .createPostponedTask( [this, errorDescription]( Services::PostponedTask& ) {
                onFailedToDownloadMeshbots( errorDescription, "templates" );
            } )
            ->start( std::chrono::seconds( 0 ), false );
    };

    m_localNotificationDownloader.downloadFileAsync(
        m_meshbotsUrl, m_downloadedMeshbotsArchivePath, processSuccess, processFailure );
}

void LocalNotifications::onSuccessFullDownloadTcds() {
    LOGGER_DEBUG_RAW( "TCDS file Download succeeded, now extracting..." );
    updateTcdsRecordInfo( Status::Downloaded );

    ReturnAction deleteArchiveAction( [this]() { filesystem::deleteFile( m_downloadedGenericDataArchivePath ); } );

    if( m_isUpdate ) {
        sendProgressBroadcast( "progress", 20, "cloudData", "download" );
    }

    try {
        auto fileSize = filesystem::getFileSize( m_downloadedGenericDataArchivePath );
        if( 0 == fileSize ) {
            LOGGER_ERROR_RAW( cc::error( "TCDS data archive file is zero length." ) );
            if( m_isUpdate ) {
                sendProgressBroadcast( "failed", 20, "cloudData", "download", "data archive file is zero length" );
            }
            return;
        }

        if( !filesystem::createFolder( m_intermediateGenericDataDirectory ) ) {
            LOGGER_ERROR_RAW( cc::error( "Unable to create TCDS intermediate directory: " ) +
                              cc::note( m_intermediateGenericDataDirectory ) );
            if( m_isUpdate ) {
                sendProgressBroadcast(
                    "failed", 30, "cloudData", "extract", "Unable to create intermediate directory" );
            }
            return;
        }
        ReturnAction deleteDirectoryAction(
            [this]() { filesystem::deleteFolder( m_intermediateGenericDataDirectory ); } );

        tryUnpackArchive( m_downloadedGenericDataArchivePath, m_intermediateGenericDataDirectory );
        LOGGER_DEBUG_RAW( cc::debug( "tcds request archive file has been extract successfully in the directory: " ) +
                          m_intermediateGenericDataDirectory );
        if( m_isUpdate ) {
            sendProgressBroadcast( "progress", 30, "cloudData", "extract" );
        }

        if( m_localNotificationTask ) {
            m_localNotificationTask->stop();
        }
        m_locationDirectory = m_intermediateGenericDataDirectory + "/data/location";
        m_partnerDirectory = m_intermediateGenericDataDirectory + "/data/partner";
        m_usersDirectory = m_intermediateGenericDataDirectory + "/data/users";
        m_settingsDirectory = m_intermediateGenericDataDirectory + "/data/settings";
        m_protectDirectory = m_intermediateGenericDataDirectory + "/data/protect";
        m_registeredDevicesDirectory = m_intermediateGenericDataDirectory + "/data/registered_devices";
        startCloudDataSegmentsDeserialization();

        if( m_isUpdate ) {
            sendProgressBroadcast( "progress", 40, "cloudData", "deploy" );
        }

        if( !deployMeshbots() ) {
            updateTcdsRecordInfo( Status::Deploying, "deploying of cloud data has failed" );
            if( m_isUpdate ) {
                sendProgressBroadcast( "failed", 40, "cloudData", "deploy", "Deploying failed" );
            }
            return;
        }

        // NOTE: This method should be already called on deploying stage
        // from the scenes manager via remote interface
        LocalNotification::Utils::setActiveCloudDataVersion(
            m_service.getDBClient(), m_tcdsNextVersion, m_cloudFileNextVersion );
        m_tcdsCurrentVersion = m_tcdsNextVersion;

        LocalNotification::Utils::removeUnusedCloudDataVersions( m_service.getDBClient() );
        if( m_isUpdate ) {
            sendProgressBroadcast( "progress", 100, "cloudData", "deployed" );
        }
        updateTcdsRecordInfo( Status::Deployed, "Cloud data has deployed" );
    }
    catch( const LocalNotification::ArchiveError& e ) {
        updateTcdsRecordInfo( Status::DeArchived, "deArchiving of cloud data has been failed" );
        if( m_isUpdate ) {
            sendProgressBroadcast( "failed", 30, "cloudData", "extract", e.what() );
        }
        return;
    }
}

void LocalNotifications::onSuccessFullDownloadMeshbots() {
    LOGGER_DEBUG_RAW( "Meshbot file Download succeeded, now extracting..." );
    updateTcdsRecordInfo( Status::Downloaded );

    ReturnAction deleteArchiveAction( [this]() { filesystem::deleteFile( m_downloadedMeshbotsArchivePath ); } );

    if( m_isUpdate ) {
        sendProgressBroadcast( "progress", 20, "templates", "download" );
    }
    try {
        auto fileSize = filesystem::getFileSize( m_downloadedMeshbotsArchivePath );
        if( 0 == fileSize ) {
            LOGGER_ERROR_RAW( cc::error( "Meshbots archive file is zero length." ) );
            if( m_isUpdate ) {
                sendProgressBroadcast( "failed", 20, "templates", "download", "meshbots archive file is zero length" );
            }
            return;
        }

        if( !filesystem::createFolder( m_intermediateMeshbotsDirectory ) ) {
            LOGGER_ERROR_RAW( cc::error( "Unable to create TCDS meshbots intermediate directory: " ) +
                              cc::note( m_intermediateMeshbotsDirectory ) );
            if( m_isUpdate ) {
                sendProgressBroadcast(
                    "failed", 30, "templates", "extract", "Unable to create intermediate directory" );
            }
            return;
        }
        ReturnAction deleteDirectoryAction( [this]() { filesystem::deleteFolder( m_intermediateMeshbotsDirectory ); } );

        if( m_isUpdate ) {
            sendProgressBroadcast( "progress", 30, "templates", "extract" );
        }

        tryUnpackArchive( m_downloadedMeshbotsArchivePath, m_intermediateMeshbotsDirectory );
        LOGGER_DEBUG_RAW( cc::debug( "Meshbot request archive file has been extract successfully in the directory: " ) +
                          m_intermediateMeshbotsDirectory );

        if( m_localNotificationTask ) {
            m_localNotificationTask->stop();
        }
        startMeshbotsDataSegmentsDeserialization();

        if( m_isUpdate ) {
            sendProgressBroadcast( "progress", 40, "templates", "deploy" );
        }

        if( !deployMeshbots() ) {
            updateTcdsRecordInfo( Status::Deploying, "deploying of meshbots has failed" );
            if( m_isUpdate ) {
                sendProgressBroadcast( "failed", 40, "templates", "deploy", "Deploy failed" );
            }
            return;
        }

        // NOTE: This method should be already called on deploying stage
        // from the scenes manager via remote interface
        LocalNotification::Utils::setActiveMeshbotsDataVersion(
            m_service.getDBClient(), m_meshbotsNextVersion, m_meshbotsFileNextVersion );
        m_meshbotsCurrentVersion = m_meshbotsNextVersion;

        LocalNotification::Utils::removeUnusedMeshbotsVersions( m_service.getDBClient() );
        if( m_isUpdate ) {
            sendProgressBroadcast( "progress", 100, "templates", "deployed" );
        }
        updateTcdsRecordInfo( Status::Deployed, "Meshbots has deployed" );
    }
    catch( const LocalNotification::ArchiveError& e ) {
        updateTcdsRecordInfo( Status::DeArchived, "deArchiving of meshbot data has been failed" );
        if( m_isUpdate ) {
            sendProgressBroadcast( "failed", 30, "templates", "extract", e.what() );
        }
        return;
    }
}

void LocalNotifications::onFailedToDownloadGenericData( const std::string& errorDescription,
                                                        const std::string& source ) {
    LOGGER_ERROR_RAW( cc::error( "Failed to download tcds request archive file from url: " ) + cc::note( m_tcdsUrl ) +
                      cc::error( " , error details: " ) + cc::note( errorDescription ) );
    filesystem::deleteFile( m_downloadedGenericDataArchivePath );
    m_downloadAttempts++;
    if( m_isUpdate && m_downloadAttempts >= 5 ) {
        updateTcdsRecordInfo( Status::Downloading, "downloading failed for source: " + source );
        sendProgressBroadcast( "failed", 20, source, "download", errorDescription );
    }
}

void LocalNotifications::onFailedToDownloadMeshbots( const std::string& errorDescription, const std::string& source ) {
    LOGGER_ERROR_RAW( cc::error( "Failed to download tcds request archive file from url: " ) + cc::note( m_tcdsUrl ) +
                      cc::error( " , error details: " ) + cc::note( errorDescription ) );
    filesystem::deleteFile( m_downloadedMeshbotsArchivePath );
    m_downloadAttempts++;
    if( m_isUpdate && m_downloadAttempts >= 5 ) {
        updateTcdsRecordInfo( Status::Downloading, "downloading failed for source: " + source );
        sendProgressBroadcast( "failed", 20, source, "download", errorDescription );
    }
}

bool LocalNotifications::handleHubOfflineCloudDataUpdateRequest( const JsonObject& message,
                                                                 const std::string& uuid,
                                                                 const std::string& sender ) {
    LOGGER_DEBUG_RAW( "hub offline cloud data update request received: " + cc::j( message ) );
    JsonObject answer;
    bool result = processErrorChecking( message, answer );
    if( !result ) {
        m_service.sendAsyncResponse( sender, answer, uuid );
        return false;
    }
    m_isUpdate = true;
    processCloudDataUpdate( message, answer );
    processMeshbotsDataUpdate( message, answer );
    m_service.sendAsyncResponse( sender, answer, uuid );
    return result;
}

void LocalNotifications::sendProgressBroadcast( const std::string& status,
                                                int completed,
                                                const std::string& source,
                                                const std::string& action,
                                                const std::string& error ) {
    JsonObject params = JSON( "status" << status << "completed" << completed << "action" << action << "source" << source
                                       << "error" << error );

    m_service.sendBroadcast( HUB_OFFLINE_CLOUD_DATA_UPDATE_PROGRESS, params );
}

void LocalNotifications::startCloudDataSegmentsDeserialization() {
    {
        auto filePath = m_intermediateGenericDataDirectory + "/info.json";
        if( !filesystem::isFileExist( filePath ) ) {
            throw LocalNotification::DeserializationError( "Unable to locate file: " + filePath );
        }

        auto data = processFileDeserialization( filePath );
        if( !data.empty() ) {
            if( m_isUpdate ) {
                sendProgressBroadcast( "failed", 40, "cloudData", "parse" );
            }
            return;
        }
        m_cloudFileNextVersion = json_get_int( data, "current_version" );
    }

    LocalNotification::LocationDeserializer locationDeserializer( m_service.getDBClient() );
    {
        auto filePath = m_locationDirectory + "/location.json";
        if( !filesystem::isFileExist( filePath ) ) {
            throw LocalNotification::DeserializationError( "Unable to locate file: " + filePath );
        }

        auto data = processFileDeserialization( filePath );
        locationDeserializer.fromJson( data );
    }

    LocalNotification::UserDeserializer userDeserializer( m_service.getDBClient() );
    {
        auto files = filesystem::getDirectoryFileNames( m_usersDirectory );
        if( !files.empty() ) {
            auto data = processFileDeserialization( files.at( 0 ) );  // TODO: Why only first user?
            userDeserializer.fromJson( data );
        }
    }

    LocalNotification::PartnerDeserializer partnerDeserializer( m_service.getDBClient() );
    {
        auto data = JSON_EMPTY_OBJ;
        auto filePath = m_partnerDirectory + "/partner.json";

        if( filesystem::isFileExist( filePath ) ) {
            data[ "partner" ] = processFileDeserialization( filePath );
        }

        filePath = m_partnerDirectory + "/custom.json";
        if( filesystem::isFileExist( filePath ) ) {
            auto customData = processFileDeserialization( filePath );
            if( !customData.empty() ) {
                data[ "custom" ] = customData;
            }
        }
        partnerDeserializer.fromJson( data );
    }

    LocalNotification::SettingsDeserializer settingsDeserializer( m_service.getDBClient() );
    if( filesystem::isDirectoryExist( m_settingsDirectory ) ) {
        auto data = JSON_EMPTY_OBJ;
        auto filePath = m_settingsDirectory + "/oem.json";
        if( filesystem::isFileExist( filePath ) ) {
            data[ "oem" ] = processFileDeserialization( filePath );
        }

        filePath = m_settingsDirectory + "/local_notifications.json";
        if( filesystem::isFileExist( filePath ) ) {
            auto localNotificationsSettings = processFileDeserialization( filePath );
            data[ "local_notifications" ] = localNotificationsSettings;
        }
        settingsDeserializer.fromJson( data );
    }

    LocalNotification::ProtectDeserializer protectDeserializer( m_service.getDBClient() );
    if( filesystem::isDirectoryExist( m_protectDirectory ) ) {
        auto data = JSON_EMPTY_OBJ;
        auto filePath = m_protectDirectory + "/protect.json";
        if( filesystem::isFileExist( filePath ) ) {
            data = processFileDeserialization( filePath );
        }
        protectDeserializer.fromJson( data );
    }

    LocalNotification::RegisteredDevicesDeserializer registeredDevicesDeserializer( m_service.getDBClient() );
    if( filesystem::isDirectoryExist( m_registeredDevicesDirectory ) ) {
        auto files = filesystem::getDirectoryFileNames( m_registeredDevicesDirectory );
        if( !files.empty() ) {
            auto data = processFileDeserialization( files[ 0 ] );  // TODO: why only one file?
            registeredDevicesDeserializer.fromJson( data );
        }
    }

    locationDeserializer.addRecordToDb( m_tcdsNextVersion, m_cloudFileNextVersion );
    userDeserializer.addRecordToDb( m_tcdsNextVersion, m_cloudFileNextVersion );
    partnerDeserializer.addRecordToDb( m_tcdsNextVersion, m_cloudFileNextVersion );
    settingsDeserializer.addRecordToDb( m_tcdsNextVersion, m_cloudFileNextVersion );
    protectDeserializer.addRecordToDb( m_tcdsNextVersion, m_cloudFileNextVersion );
    registeredDevicesDeserializer.addRecordToDb( m_tcdsNextVersion, m_cloudFileNextVersion );

    LocalNotification::Utils::addCloudDataInfo(
        m_service.getDBClient(), m_tcdsNextVersion, m_cloudFileNextVersion, m_tcdsUrl, m_cloudS3Id );
    updateTcdsRecordInfo( Status::DeArchived );
    if( m_isUpdate ) {
        sendProgressBroadcast( "progress", 40, "cloudData", "parse" );
    }
}

JsonObject LocalNotifications::processFileDeserialization( const std::string& filePath ) {
    JsonObject content;

    if( filesystem::isFileExist( filePath ) ) {
        try {
            std::ifstream file( filePath );
            file >> content;
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "Error loading file: " + cc::note( filePath ) ) + ex.what() );
        }
    }
    else {
        LOGGER_ERROR_RAW( cc::error( "info.json file doesn't exists" ) );
    }
    return content;
}

void LocalNotifications::processCloudDataUpdate( const JsonObject& message, JsonObject& answer ) {
    if( json_contains( message, "current_version" ) ) {
        auto currentVersion = json_get_int( message, "current_version" );
        if( currentVersion > m_tcdsCurrentVersion ) {
            if( m_cloudDataUpdateTask && m_cloudDataUpdateTask->isActive() ) {
                m_cloudDataUpdateTask->stop();
                m_tcdsNextVersion = currentVersion;
                m_tcdsUrl = json_get_str( message, "url" );
                m_cloudS3Id = json_get_str( message, "s3_id" );

                auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
                m_intermediateGenericDataDirectory = m_localNotificationDirectory + "/tcds_" + date;
                m_downloadedGenericDataArchivePath = m_intermediateGenericDataDirectory + ".tgz";
                if( !m_cloudDataHandler->isQueueEmpty() ) {
                    m_cloudDataHandler->clearQueue();
                    setError( answer, Errors::EZLO_TCDS_UPDATED, "Tcds data is updating for new version" );
                }
                m_cloudDataUpdateTask->start( CloudDataUpdateTaskPeriod, false );
            }
            else {
                m_cloudDataHandler->addRecordToQueue( JSON( "current_version"
                                                            << json_get_int( message, "current_version" ) << "url"
                                                            << json_get_str( message, "url" ) << "s3_id"
                                                            << json_get_str( message, "s3_id" ) ) );
            }
        }
    }
}

void LocalNotifications::processMeshbotsDataUpdate( const JsonObject& message, JsonObject& answer ) {
    if( json_contains( message, "meshbots_current_version" ) ) {
        int meshbotsCurrentVersion = json_get_int( message, "meshbots_current_version" );
        if( meshbotsCurrentVersion > m_meshbotsCurrentVersion ) {
            if( m_meshbotTemplateUpdateTask && m_meshbotTemplateUpdateTask->isActive() ) {
                m_meshbotTemplateUpdateTask->stop();
                m_meshbotsCurrentVersion = meshbotsCurrentVersion;
                m_meshbotsUrl = json_get_str( message, "meshbots_url" );
                m_meshbotS3Id = json_get_str( message, "meshbots_s3_id" );

                auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
                m_intermediateMeshbotsDirectory = m_localNotificationDirectory + "/meshbots_" + date;
                m_downloadedMeshbotsArchivePath = m_intermediateMeshbotsDirectory + ".tgz";
                if( isStorageLimitReached() ) {
                    if( !m_meshbotsTemplateHandler->isQueueEmpty() ) {
                        m_meshbotsTemplateHandler->clearQueue();
                        setError( answer, Errors::EZLO_TCDS_UPDATED, "Tcds data is updating for new version" );
                    }
                    m_meshbotTemplateUpdateTask->start( MeshbotsTemplateUpdateTaskPeriod, false );
                }
            }
            else {
                m_meshbotsTemplateHandler->addRecordToQueue( JSON(
                    "meshbots_current_version" << json_get_int( message, "meshbots_current_version" ) << "meshbots_url"
                                               << json_get_str( message, "meshbots_url" ) << "meshbots_s3_id"
                                               << json_get_str( message, "meshbots_s3_id" ) ) );
            }
        }
    }
}

void LocalNotifications::startMeshbotsDataSegmentsDeserialization() {
    auto filePath = m_intermediateMeshbotsDirectory + "/fileversion.json";
    if( !filesystem::isFileExist( filePath ) ) {
        throw LocalNotification::DeserializationError( "Unable to locate file: " + filePath );
    }

    auto data = processFileDeserialization( filePath );
    m_meshbotsFileNextVersion = json_get_int( data, "version" );

    auto meshbotsTemplatesDirectory = m_intermediateMeshbotsDirectory + "/data/meshbot_templates";
    if( !filesystem::isDirectoryExist( filePath ) ) {
        throw LocalNotification::DeserializationError( "Unable to locate directory: " + meshbotsTemplatesDirectory );
    }

    auto templateFiles = filesystem::findAllFiles( meshbotsTemplatesDirectory );
    if( templateFiles.empty() ) {
        throw LocalNotification::DeserializationError( "Unable to locate template files in directory: " +
                                                       meshbotsTemplatesDirectory );
    }

    std::vector< LocalNotification::MeshbotUuidDeserializer > meshbots;
    meshbots.reserve( templateFiles.size() );
    for( auto& file : templateFiles ) {
        filePath = meshbotsTemplatesDirectory.append( "/" + file );
        data = processFileDeserialization( filePath );
        meshbots.emplace_back( m_service.getDBClient() );
        meshbots.back().fromJson( data );
    }

    for( auto& meshbot : meshbots ) {
        meshbot.addRecordToDb( m_meshbotsNextVersion, m_meshbotsFileNextVersion );
    }

    LocalNotification::Utils::addMeshbotsDataInfo(
        m_service.getDBClient(), m_meshbotsNextVersion, m_meshbotsFileNextVersion, m_meshbotsUrl, m_meshbotS3Id );
    updateTcdsRecordInfo( Status::DeArchived );

    if( m_isUpdate ) {
        sendProgressBroadcast( "progress", 40, "templates", "parse" );
    }
}

void LocalNotifications::initTcdsRecordDataToDb() {
    using namespace LocalNotification::Resources::DB;

    if( !m_service.getDBClient().has( Table::LocalNotificationsDB, JSON( Fields::Id << Fields::TcdsDataInfo ) ) ) {
        m_service.getDBClient().insert(
            Table::LocalNotificationsDB,
            JSON( Fields::Id << Fields::TcdsDataInfo << "status" << enumToString( Status::Unknown ) << "deployed" << 0
                             << "process_error"
                             << "" ) );
    }
}

std::string LocalNotifications::enumToString( LocalNotifications::Status status ) {
    if( status == Status::Unknown ) {
        return "unknown";
    }
    else if( status == Status::Downloading ) {
        return "downloading";
    }
    else if( status == Status::Downloaded ) {
        return "downloaded";
    }
    else if( status == Status::DeArchived ) {
        return "de_archived";
    }
    else if( status == Status::Deploying ) {
        return "deploying";
    }
    else if( status == Status::Deployed ) {
        return "deployed";
    }
    return "unknown";
}

void LocalNotifications::updateTcdsRecordInfo( LocalNotifications::Status status, const std::string& error ) {
    using namespace LocalNotification::Resources::DB;

    if( status != Status::Deployed ) {
        m_service.getDBClient().update(
            Table::LocalNotificationsDB,
            JSON( Fields::Id << Fields::TcdsDataInfo ),
            JSON( "$set" << JSON( "status" << enumToString( status ) << "process_error" << error ) ) );
    }
    else {
        auto currentTimestamp = std::chrono::system_clock::now();
        auto timeSeconds = std::chrono::system_clock::to_time_t( currentTimestamp );
        m_service.getDBClient().update( Table::LocalNotificationsDB,
                                        JSON( Fields::Id << Fields::TcdsDataInfo ),
                                        JSON( "$set" << JSON( "status" << enumToString( status ) << "process_error"
                                                                       << error << "deployed" << timeSeconds ) ) );
    }
}

JsonObject LocalNotifications::handleHubOfflineCloudDataInfoGet( const JsonObject& params ) {
    JsonObject result;
    try {
        using namespace LocalNotification::Resources::DB;

        result = m_service.getDBClient().findOne(
            Table::LocalNotificationsDB, JSON( Fields::Id << Fields::TcdsDataInfo ), nullptr );
        auto cloudDataInfo = LocalNotification::Utils::getActiveCloudDataInfo( m_service.getDBClient() );
        auto meshbotDataInfo = LocalNotification::Utils::getActiveMeshbotsDataInfo( m_service.getDBClient() );

        auto error = json_get_str( result, "process_error" );
        if( !error.empty() ) {
            throw LocalNotificationsException( json_get_str( result, "process_error" ) );
        }
        result.erase( "_id" );
        result[ "cloudData" ] = prepareDataForInfoGetRequest( cloudDataInfo );
        result[ "meshbotTemplates" ] = prepareDataForInfoGetRequest( meshbotDataInfo );
        result.erase( "process_error" );

        return result;
    }
    catch( const LocalNotificationsException& ex ) {
        JsonObject error;
        setError( error, Errors::LOCAL_NOTIFICATION_TCDS_DATA_PROCESSING_ERROR, ex.what() );
        return error;
    }

    catch( const std::exception& ex ) {
        JsonObject error;
        setError( error, Errors::DB_ERROR, ex.what() );
        return error;
    }
}

bool LocalNotifications::isStorageLimitReached() {
    auto fileSystemSize = filesystem::getFolderSize( m_localNotificationDirectory );
    const uint64_t sizeThreshold = m_fileSizeLimit * 1024 * 1024;  // 100 MB in bytes
    if( fileSystemSize >= sizeThreshold ) {
        updateTcdsRecordInfo( Status::Downloading,
                              "file storage limit exceed, size: " + std::to_string( fileSystemSize ) + " bytes" );
        return true;
    }
    return false;
}

JsonObject LocalNotifications::prepareDataForInfoGetRequest( const JsonObject& data ) {
    JsonObject result;
    result[ "currentVersion" ] = json_get_int( data, "current_version" );
    result[ "fileVersion" ] = json_get_str( data, "file_version" );
    result[ "modified" ] = json_get_str( data, "modified" );
    result[ "url" ] = json_get_str( data, "url" );
    result[ "s3Id" ] = json_get_str( data, "s3_id" );
    auto status = json_get_str( data, "status" );  // TODO check if status needed
    if( status.empty() )
        result[ "status" ] = "unknown";
    else
        result[ "status" ] = status;
    return result;
}

bool LocalNotifications::processErrorChecking( const JsonObject& request, JsonObject& answer ) {
    if( isStorageLimitReached() ) {
        setError( answer, Errors::LOCAL_NOTIFICATION_STORAGE_IS_FULL, "file storage size limit reached" );
        return false;
    }
    if( json_contains( request, "current_version" ) ) {
        auto currentVersion = json_get_int( request, "current_version" );
        if( currentVersion > m_tcdsCurrentVersion ) {
            auto url = json_get_str( request, "url" );
            if( url.empty() ) {
                setError( answer, Errors::WRONG_FIELD, "url is empty" );
                return false;
            }
        }
        else {
            if( !json_contains( request, "meshbots_current_version" ) ) {
                setError( answer, Errors::WRONG_FIELD, "current_version is not updated one" );
                return false;
            }
        }
    }
    if( json_contains( request, "meshbots_current_version" ) ) {
        auto currentVersion = json_get_int( request, "meshbots_current_version" );
        if( currentVersion > m_meshbotsCurrentVersion ) {
            auto url = json_get_str( request, "meshbots_url" );
            if( url.empty() ) {
                setError( answer, Errors::WRONG_FIELD, "meshbots_url is empty" );
                return false;
            }
        }
        else {
            setError( answer, Errors::WRONG_FIELD, "meshbots_current_version is not updated one" );
            return false;
        }
    }

    return true;
}

void LocalNotifications::skippingToLatestCloudDataRequest() {
    if( !m_cloudDataHandler->isQueueEmpty() ) {
        auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
        auto cloudData = m_cloudDataHandler->fetchLatestRecordFromQueue();
        m_cloudDataHandler->clearQueue();
        m_tcdsNextVersion = json_get_int( cloudData, "current_version" );
        m_tcdsUrl = json_get_str( cloudData, "url" );
        m_cloudS3Id = json_get_str( cloudData, "s3_id" );
        m_intermediateGenericDataDirectory = m_localNotificationDirectory + "/tcds_" + date;
        m_downloadedGenericDataArchivePath = m_intermediateGenericDataDirectory + ".tgz";
        m_cloudDataUpdateTask->start( CloudDataUpdateTaskPeriod, false );
    }
}

void LocalNotifications::skippingToLatestMeshbotsTemplatesRequest() {
    if( !m_meshbotsTemplateHandler->isQueueEmpty() ) {
        auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
        auto meshbotTemplates = m_meshbotsTemplateHandler->fetchLatestRecordFromQueue();
        m_meshbotsTemplateHandler->clearQueue();
        m_meshbotsNextVersion = json_get_int( meshbotTemplates, "meshbots_current_version" );
        m_meshbotsUrl = json_get_str( meshbotTemplates, "meshbots_url" );
        m_meshbotS3Id = json_get_str( meshbotTemplates, "meshbots_s3_id" );
        m_intermediateMeshbotsDirectory = m_localNotificationDirectory + "/meshbots_" + date;
        m_downloadedMeshbotsArchivePath = m_intermediateMeshbotsDirectory + ".tgz";
        m_meshbotTemplateUpdateTask->start( MeshbotsTemplateUpdateTaskPeriod, false );
    }
}

void LocalNotifications::tryUnpackArchive( const std::string& archivePath, const std::string& outputDirectory ) {
    try {
        System::untarArchive( archivePath, outputDirectory );
        return;
    }
    catch( const System::SystemError& ex ) {
        LOGGER_ERROR_RAW( cc::error( "TCDS compressed archive has not been extract successfully, path: " +
                                     cc::note( archivePath ) + ", error: " ) +
                          ex.what() );
    }

    LOGGER_INFO( cc::info( "Now trying to extracting uncompressed archive" ) );

    try {
        System::untarUnCompressedArchive( archivePath, outputDirectory );
        return;
    }
    catch( const System::SystemError& ex ) {
        LOGGER_ERROR_RAW( cc::error( "TCDS uncompressed archive has not been extract successfully, path: " +
                                     cc::note( archivePath ) + ", error: " ) +
                          ex.what() );
    }

    throw LocalNotification::ArchiveError( "Unable to unpack archive: " + archivePath +
                                           ", output directory: " + outputDirectory );
}

bool LocalNotifications::deployMeshbots() {
    // TBD EFWR-4995: Send request to scenes manager
    return true;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
