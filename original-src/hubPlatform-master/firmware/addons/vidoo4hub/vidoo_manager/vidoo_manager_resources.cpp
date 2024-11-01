#include "vidoo4hub/vidoo_manager/vidoo_manager_resources.h"

#include "vidoo4hub/vidoo_device/vidoo_device.h"
#include "vidoo4hub/camera_manager/camera_manager.h"

namespace HubPlatform {
namespace Vidoo4Hub {

VidooManagerResources::VidooManagerResources( VidooDevice& vidooDevice, CameraManager& cameraManager )
: m_vidooDevice( vidooDevice )
, m_cameraManager( cameraManager ) {}

std::string VidooManagerResources::getDetectableItemId( DetectableTypes::Type type ) {
    return m_vidooDevice.getDetectableItemId( type );
}

std::map< std::string, VidooCamera > VidooManagerResources::getCameraList() {
    return m_cameraManager.getCameras();
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
