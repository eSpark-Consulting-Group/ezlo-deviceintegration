#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class ICoreManager;
class ICameraManager;
class IStreamManager;
class IVidooManager;
class IVidooClient;

class VidooMediator {
public:
    VidooMediator( ICoreManager& coreManager,
                   IVidooClient& vidooClient,
                   ICameraManager& cameraManager,
                   IStreamManager& streamManager,
                   IVidooManager& vidooManager );

public:
    void handleVidooMessage( const JsonObject& message );
    void handleItemUpdate( const JsonObject& message );
    void handleCamerasStartRtspStreaming( const JsonObject& message );
    void handleCamerasStopRtspStreaming( const JsonObject& message );

    // MQTT handlers
    void handleUpdateCameraList( const Json& params );
    void handleUpdateObjectList( const Json& params );
    void handleUpdatePeopleList( const Json& params );
    void handleUpdateQRCodeList( const Json& params );
    void handleUpdateHotzoneList( const Json& params );
    void handleMotion( const Json& params );
    void handleHotzoneMotion( const Json& params );

private:
    void handleSnapshotItemUpdate( const std::string& cameraId,
                                   const std::string& itemId,
                                   const JsonObject& itemValue );
    void handleStopRecordingItemUpdate( const std::string& cameraId,
                                        const std::string& itemId,
                                        const JsonObject& itemValue );
    void handleMakeRecordingItemUpdate( const std::string& cameraId,
                                        const std::string& itemId,
                                        const JsonObject& itemValue );
    void handleRequestCameraStreamsList();

private:
    ICoreManager& m_coreManager;
    IVidooClient& m_vidooClient;
    ICameraManager& m_cameraManager;
    IStreamManager& m_streamManager;
    IVidooManager& m_vidooManager;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
