#pragma once
#include "service_interfaces/postponed_task.h"
#include "json_utils/json_fwd.h"
#include "local_notification_downloader.h"
#include "multiple_data_handler.h"
#include <mutex>
#include <string>
#include <set>
#include <chrono>
#include <atomic>
#include <utility>
#include <vector>
#include <queue>

namespace HubPlatform {
namespace Services {
class Service;
}

namespace CloudDevices {

class LocalNotificationsException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class LocalNotifications {
    enum class Status { Unknown = 0, Downloading, Downloaded, DeArchived, Deploying, Deployed };

public:
    LocalNotifications( Services::Service& service );
    ~LocalNotifications();

    void sendTcdsCurrentVersionRequest();
    bool handleHubOfflineCloudDataUpdateRequest( const JsonObject& message,
                                                 const std::string& uuid,
                                                 const std::string& sender );
    JsonObject handleHubOfflineCloudDataInfoGet( const JsonObject& params );

private:
    void handleTcdsRequestResponse( const JsonObject& message );
    void onSuccessFullDownloadTcds();
    void onFailedToDownloadGenericData( const std::string& errorDescription, const std::string& source );
    void onFailedToDownloadMeshbots( const std::string& errorDescription, const std::string& source );
    void startDownloadAndExtractTcds();
    static JsonObject localNotificationRequestBuilder();
    void tcdsRequestTaskRestart();
    void handleTcdsRequestTimeout();
    void sendProgressBroadcast( const std::string& status,
                                int completed,
                                const std::string& source,
                                const std::string& action = "",
                                const std::string& error = "" );
    void startCloudDataSegmentsDeserialization();
    static JsonObject processFileDeserialization( const std::string& filePath );
    void startDownloadAndExtractMeshbots();
    void onSuccessFullDownloadMeshbots();
    void processCloudDataUpdate( const JsonObject& message, JsonObject& answer );
    void processMeshbotsDataUpdate( const JsonObject& message, JsonObject& answer );
    void startMeshbotsDataSegmentsDeserialization();
    void initTcdsRecordDataToDb();
    static std::string enumToString( Status status );
    void updateTcdsRecordInfo( Status status, const std::string& error = "" );
    bool isStorageLimitReached();
    JsonObject prepareDataForInfoGetRequest( const JsonObject& data );
    bool processErrorChecking( const JsonObject& request, JsonObject& answer );
    void skippingToLatestCloudDataRequest();
    void skippingToLatestMeshbotsTemplatesRequest();
    void tryUnpackArchive( const std::string& archivePath, const std::string& outputDirectory );
    bool deployMeshbots();

private:
    const static std::chrono::minutes LocalNotificationRequestPeriod;
    const static std::chrono::seconds CloudDataUpdateTaskPeriod;
    const static std::chrono::seconds MeshbotsTemplateUpdateTaskPeriod;

private:
    Services::Service& m_service;
    Services::PostponedTaskPtr m_localNotificationTask;
    int m_tcdsCurrentVersion = -1;
    int m_tcdsNextVersion = -1;
    std::string m_tcdsUrl;
    std::string m_localNotificationDirectory;
    std::string m_intermediateGenericDataDirectory;
    std::string m_intermediateMeshbotsDirectory;
    std::string m_downloadedGenericDataArchivePath;
    std::string m_downloadedMeshbotsArchivePath;
    int m_retryCount;
    LocalNotificationsDownloader m_localNotificationDownloader;
    int m_downloadAttempts;
    std::string m_locationDirectory;
    std::string m_partnerDirectory;
    std::string m_usersDirectory;
    int m_cloudFileNextVersion = -1;
    std::string m_settingsDirectory;
    std::string m_protectDirectory;
    std::string m_registeredDevicesDirectory;
    int m_meshbotsCurrentVersion = -1;
    int m_meshbotsNextVersion = -1;
    std::string m_meshbotsUrl;
    int m_meshbotsFileNextVersion = -1;
    int m_fileSizeLimit;
    std::string m_cloudS3Id;
    std::string m_meshbotS3Id;
    std::atomic_bool m_isUpdate;
    Services::PostponedTaskPtr m_cloudDataUpdateTask;
    Services::PostponedTaskPtr m_meshbotTemplateUpdateTask;
    Services::PostponedTaskPtr m_multipleVersionHandleTask;
    std::unique_ptr< CloudDataHandler > m_cloudDataHandler;
    std::unique_ptr< MeshbotsTemplateHandler > m_meshbotsTemplateHandler;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
