#pragma once
#include "cameras/defs.h"

#include "addon_interfaces/addon.h"
#include "cloud_storage/cloud_storage_interface.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include "cameras/live_rtsp_stream_status_notifier_interface.h"

#include "cameras/events/camera_base_event.h"
#include "cameras/events/event_publisher_interface.h"
#include "cameras/rtsp_server/rtsp_server_controller_interface.h"
#include "cameras/rtsp_server/event.h"
#include "cameras/stream_status.h"
#include "cameras/snapshot/snapshot_cache.h"

#include <map>
#include <mutex>

namespace HubPlatform {

class ConfigReader;
struct StreamParticleSettings;

namespace Cameras {

namespace UT {
class CameraAddonTestSuite;
}

class CamerasAddon final : public Addons::Addon, public ILiveRTSPStreamStatusNotifier {
public:
    using StreamId = std::string;
    using PluginId = std::string;
    using Clock = std::chrono::system_clock;
    using CachedPath = std::string;
    using ImageRemovers = std::map< CachedPath /*cachedPath*/, FileRemoverSharedPtr /*remover*/ >;

    CamerasAddon();
    ~CamerasAddon() override;

    void setUp() override;
    void tearDown() override;

    CommonUtils::AddonVersion getVersion() const override;

    int restoreBackup( const std::string& tempBackupFolder ) override;

private:
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;

protected:
    JsonObject getServiceInfo() override;

private:
    ///
    /// \brief addCameraStream creates instance in DB. Throws if already exists.
    /// \param params
    ///   {
    ///      "streamId": "_itemId_",
    ///      "streamName": "CAM1",
    ///      "rtspUrl": "rtsp://...",
    ///      "description": ""
    ///   }
    void addCameraStream( const JsonObject& params );

    ///
    /// \brief removeCameraStream removes instance from DB and stops all activities
    /// for the camera.
    /// \param params
    ///   {
    ///      "streamId": ""
    ///   }
    void removeCameraStream( const JsonObject& params );

    ///
    /// \brief createLiveRtspStream proxies live stream (rtsp) from the camera to live555,
    /// replaces existing with same streamId. Throws if reading stream from
    /// the camera was not started.
    /// \param params
    ///   {
    ///      "streamId": ""
    ///   }
    void createLiveRtspStream( const JsonObject& params );

    /// \param params
    ///   {
    ///      "streamId": ""
    ///   }
    void deleteLiveRtspStream( const JsonObject& params );

    void createStreamSavingToCloud( const JsonObject& params );
    void deleteStreamSavingToCloud( const JsonObject& params );
    void startStreamSavingToCloud( const JsonObject& params );
    void stopStreamSavingToCloud( const JsonObject& params );

    void setVideoParticleSettings( const JsonObject& params );

    void makeStreamSnapshotForCloud( const JsonObject& params );
    void makeStreamSnapshotForUI( const JsonObject& params, const std::string& uuid, const std::string& sender );

    Camera& findOrCreateCamera( const std::string& streamId );

    void updateIpAddress( const JsonObject& params );

    void handleExternalEvent( const JsonObject& params );

    void handleEventFromBus( const Services::Message& message );
    void handleCameraCloudTunnelStatusChanged( const JsonObject& params );

    void handleNetworkInterfaceStatusChanged( const JsonObject& params );

    void printStreamStatuses() const;
    Json getStreamInfo( const JsonObject& params );

    void onRTSPStreamStarted( const StreamId& streamId, const LiveRTSPStreamLink& link ) override;
    void onRTSPStreamStopped( const StreamId& streamId ) override;
    void onRTSPStreamCreationFailed( const StreamId& streamId, const std::string& errorMessage ) override;
    void onRTSPStreamError( const StreamId& streamId, StreamError error ) override;

private:
    CameraPtr createCamera( const std::string& streamId, const std::string& rtspUrl );

    void saveFileToCloud( const FilePath& filePath,
                          const ResultCallback& resultCallback,
                          const FileRemoverSharedPtr& remover,
                          const std::string& uuid,
                          const Json& metaData,
                          bool useImageCache );
    void saveRecodingToCloud( const StreamId& streamId,
                              const FilePath& filePath,
                              const ResultCallback& resultCallback,
                              const FileRemoverSharedPtr& remover,
                              const Json& metaData );
    void saveParticleToCloud( const StreamId& streamId,
                              const FilePath& filePath,
                              const ResultCallback& resultCallback,
                              const FileRemoverSharedPtr& remover,
                              const Json& metaData );

    static void applyNewLogLevel( int logLevel );

    static Json createError( const std::string& errorMessage );
    static Json createMetaDataForCloudUploading( const Json& userMetaData );

    void deleteStreamSavingToCloud( const std::string& streamId );

    void setConfigIntOption( ConfigReader& configReader,
                             std::function< void( int ) > setter,
                             const char* optionPath,
                             int defaultValue );

    void makePreviewParticle( Camera& camera, const StreamId& streamId );
    bool isParticleRecordingsLimitReached() const;

    std::string updateLiveRtspStreamLink( std::string link );

    PluginId getPluginId( const StreamId& streamId ) const;
    StreamParticleSettings findParticleSettings( const StreamId& streamId ) const;
    StreamStatus* findStreamStatus( const StreamId& streamId );

    void startLiveStreams();
    void readConnectedNetworkInterfaces( const std::vector< Json >& connectedInterfaces );

    void onSubscriberAdded( const PluginId& pluginId ) override;
    void emitEventToPlugin( const StreamId& streamId, const CameraBaseEvent& event );
    void emitEventToAllPlugins( const CameraBaseEvent& event );

    JsonObject getCameraStreams();

    void removeCachedImageIfExpired( const std::string& streamId );
    void makeStreamSnapshotForCloudInternal( const JsonObject& params );
    std::string getDeviceId( const std::string& streamId );

private:
    using ConnectedNetworkIntefaces = std::set< std::string >;
    using Cameras = std::map< StreamId, CameraPtr >;
    using PluginIds = std::map< StreamId, PluginId >;

    struct VideoParticleData {
        VideoParticleData( FilePath filePath,
                           ResultCallback resultCallback,
                           FileRemoverSharedPtr remover,
                           Json metaData );
        FilePath m_filePath;
        ResultCallback m_resultCallback;
        FileRemoverSharedPtr m_remover;
        Json m_metaData;
    };

    mutable std::recursive_mutex m_mutex;
    Cameras m_cameras;

    mutable std::mutex m_pluginIdsMutex;
    PluginIds m_pluginIds;

    std::unique_ptr< IRtspServerController > m_rtspServer;

    std::unique_ptr< Common::CloudStorageInterface::RemoteInterface > m_cloudStorageInterface;
    std::unique_ptr< Network::RemoteInterface > m_networkRemoteInterface;

    std::unique_ptr< IEventPublisher< EventBase > > m_serverEventPublisher;

    ConnectedNetworkIntefaces m_connectedNetworkInterfaces;
    std::map< PluginId, StreamParticleSettings > m_particlesSettings;
    std::map< StreamId, std::unique_ptr< VideoParticleData > > m_videoParticlesData;

    mutable std::mutex m_streamStatusMutex;
    std::map< StreamId, StreamStatus > m_streamStatuses;

    ImageRemovers m_imageRemovers;

    std::string m_currentIpAddress;
    uint16_t m_rtspServerPort{ 5554 };

    Services::PostponedTaskPtr m_streamStatusPrinter;
    Services::PostponedTaskPtr m_cachedSnapshotsProcessing;

    SnapshotCachePtr m_snapshotCache;
    int m_makeSnapshotTimeout = 20;

    static constexpr int MinParticleResolutionDivisor{ 4 };
    static constexpr std::chrono::seconds DefaultParticlePreRecordTime{ 2 };
    static constexpr std::chrono::seconds DefaultParticleRecordTime{ 3 };
    static constexpr std::chrono::minutes StreamStatusesFirstPrintTime{ 1 };
    static constexpr std::chrono::hours StreamStatusesPrintPeriod{ 1 };

    friend class UT::CameraAddonTestSuite;
};

}  // namespace Cameras
}  // namespace HubPlatform
