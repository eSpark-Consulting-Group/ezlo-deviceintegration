#pragma once

#include "vidoo4hub/vidoo_manager/vidoo_manager_resources_interface.h"
#include "vidoo4hub/camera/vidoo_camera.h"

#include <map>

namespace HubPlatform {
namespace UT {

class FakeVidooManagerResources : public Vidoo4Hub::IVidooManagerResources {
public:
    std::string getDetectableItemId( Vidoo4Hub::DetectableTypes::Type type ) override {
        return m_detectableItemsMap.at( type );
    }

    std::map< std::string, Vidoo4Hub::VidooCamera > getCameraList() override {
        return m_cameras;
    }

public:
    std::map< std::string, Vidoo4Hub::VidooCamera > m_cameras;
    std::map< Vidoo4Hub::DetectableTypes::Type, std::string > m_detectableItemsMap;
};

}  // namespace UT
}  // namespace HubPlatform
