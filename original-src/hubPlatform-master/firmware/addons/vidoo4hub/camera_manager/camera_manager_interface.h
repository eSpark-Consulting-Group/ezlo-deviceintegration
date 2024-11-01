#pragma once

#include "vidoo4hub/camera/vidoo_camera.h"
#include "json_utils/json_fwd.h"

#include <vector>
#include <mutex>
#include <map>

namespace HubPlatform {
namespace Vidoo4Hub {

class CameraItemsCache;

class ICameraManager {
public:
    using CameraId = std::string;
    virtual ~ICameraManager() = default;

    virtual void addCamera( std::string cameraId,
                            std::string cameraSubcategory,
                            std::string cameraName,
                            std::string rtspURL ) = 0;
    virtual void updateCamera( std::string cameraId,
                               std::string cameraSubcategory,
                               std::string cameraName,
                               std::string rtspURL ) = 0;
    virtual void deleteCamera( const std::string& cameraId ) = 0;
    virtual void updateCameraList( const std::vector< Json >& cameraJsonList ) = 0;
    virtual void setMotionStarted( const std::string& cameraId ) = 0;
    virtual void setMotionEnded( const std::string& cameraId ) = 0;
    virtual void handleStreamStarted( const std::string& streamId, const std::string& rtspURL ) = 0;
    virtual void handleStreamStopped( const std::string& streamId ) = 0;
    virtual void reloadCameras() = 0;
    virtual void cleanItemValues() = 0;

    virtual const std::map< CameraId, VidooCamera >& getCameras() = 0;
    virtual const CameraItemsCache& getCameraItemsCache() = 0;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
