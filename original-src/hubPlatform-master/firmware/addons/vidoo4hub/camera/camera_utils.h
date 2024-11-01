#pragma once

#include "vidoo4hub/camera/vidoo_camera.h"

#include "cored/remote_interface/cored_interface.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

#include <map>
#include <memory>

namespace HubPlatform {
namespace Vidoo4Hub {
namespace CameraUtils {

using CoreInterface = HubPlatform::CoreDRemoteInterface::CoreDInterface;

bool isKnownCameraSubcategory( const std::string& cameraSubcategory );

Json cameraToJson( const VidooCamera& camera );
VidooCamera cameraFromJson( const Json& j );

void storeCameraToDB( IDbClient& dbClient, const VidooCamera& camera );
void deleteCameraFromDB( IDbClient& dbClient, const std::string& cameraId );
std::unique_ptr< VidooCamera > readCameraFromDB( IDbClient& dbClient, const std::string& cameraId );
std::map< std::string, VidooCamera > readAllCamerasFromDB( IDbClient& dbClient );

std::string createCameraDevice( CoreInterface& core,
                                const std::string& gatewayId,
                                const std::string& subcategory,
                                const std::string& cameraDeviceName );
std::string createMotionSensorDevice( CoreInterface& core,
                                      const std::string& gatewayId,
                                      const std::string& cameraDeviceId,
                                      const std::string& cameraDeviceName );
std::string createSecurityThreatItem( CoreInterface& core, const std::string& motionSensorId );
std::string createMotionItem( CoreInterface& core, const std::string& motionSensorId );
std::string createHotzonesItem( CoreInterface& core, const std::string& cameraDeviceId );
std::string createSnapshotItem( CoreInterface& core, const std::string& cameraDeviceId );
std::string createMakeRecordingItem( CoreInterface& core, const std::string& cameraDeviceId );
std::string createStopRecordingItem( CoreInterface& core, const std::string& cameraDeviceId );
std::string createStreamItem( CoreInterface& core, const std::string& cameraDeviceId, const std::string& streamName );
void removeDevice( CoreInterface& core, const std::string& deviceId );
void removeItem( CoreInterface& core, const std::string& itemId );
void setMotionStarted( CoreInterface& core, const std::string& itemId );
void setMotionEnded( CoreInterface& core, const std::string& itemId );
void setStreamStarted( CoreInterface& core, const std::string& itemId, const std::string& rtspURL );
void setStreamStopped( CoreInterface& core, const std::string& itemId );
void cleanCameraItemValues( CoreInterface& core, const VidooCamera& camera );

Json getDefaultHotzoneJson();
Json getDefaultHotzoneDictionaryJson();

std::string createMotionSensorName( const std::string& cameraDeviceName );

}  // namespace CameraUtils
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
