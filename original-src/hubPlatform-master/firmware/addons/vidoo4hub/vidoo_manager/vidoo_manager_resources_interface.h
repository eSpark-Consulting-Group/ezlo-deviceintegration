#pragma once

#include "vidoo4hub/camera/vidoo_camera.h"
#include "vidoo4hub/detectable_types/detectable_types.h"
#include <map>

namespace HubPlatform {
namespace Vidoo4Hub {

class IVidooManagerResources {
public:
    virtual ~IVidooManagerResources() = default;
    virtual std::string getDetectableItemId( DetectableTypes::Type type ) = 0;
    virtual std::map< std::string, VidooCamera > getCameraList() = 0;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
