#pragma once

#include "vidoo4hub/camera_manager/camera_manager_interface.h"

#include "vidoo4hub/camera/vidoo_camera.h"
#include "vidoo4hub/stream_manager/stream_manager_interface.h"
#include "vidoo4hub/camera_manager/camera_items_cache.h"

#include "service_interfaces/service.h"

#include <mutex>
#include <map>

namespace HubPlatform {

namespace UT {
class CameraManagerTestSuite;
}  // namespace UT

namespace Vidoo4Hub {

class ICoreManager;

class CameraManager : public ICameraManager {
public:
    using CameraId = std::string;
    CameraManager( Services::Service& service,
                   ICoreManager& coreManager,
                   IStreamManager& streamManager,
                   std::string vidooDeviceId,
                   std::string coreDGatewayId );
    void addCamera( std::string cameraId,
                    std::string cameraSubcategory,
                    std::string cameraName,
                    std::string rtspURL ) override;
    void updateCamera( std::string cameraId,
                       std::string cameraSubcategory,
                       std::string cameraName,
                       std::string rtspURL ) override;
    void deleteCamera( const std::string& cameraId ) override;
    void updateCameraList( const std::vector< Json >& cameraJsonList ) override;
    void setMotionStarted( const std::string& cameraId ) override;
    void setMotionEnded( const std::string& cameraId ) override;
    void handleStreamStarted( const std::string& streamId, const std::string& rtspURL ) override;
    void handleStreamStopped( const std::string& streamId ) override;
    void reloadCameras() override;
    void cleanItemValues() override;

    const std::map< CameraId, VidooCamera >& getCameras() override;
    const CameraItemsCache& getCameraItemsCache() override;

private:
    void deleteCameraResources( const VidooCamera& camera );

private:
    Services::Service& m_service;
    ICoreManager& m_coreManager;
    IStreamManager& m_streamManager;
    const std::string m_vidooDeviceId;
    const std::string m_gatewayId;
    std::map< CameraId, VidooCamera > m_cameraList;

    CameraItemsCache m_itemsCache;

    friend class UT::CameraManagerTestSuite;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
