#pragma once

#include "vidoo4hub/vidoo_manager/vidoo_manager_resources_interface.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class VidooDevice;
class CameraManager;

class VidooManagerResources : public IVidooManagerResources {
public:
    VidooManagerResources( VidooDevice& vidooDevice, CameraManager& cameraManager );
    std::string getDetectableItemId( DetectableTypes::Type type ) override;
    std::map< std::string, VidooCamera > getCameraList() override;

private:
    VidooDevice& m_vidooDevice;
    CameraManager& m_cameraManager;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
