#pragma once

#include "vidoo4hub/vidoo_manager/vidoo_manager_interface.h"
#include "cored/remote_interface/cored_interface.h"

#include <string>
#include <vector>
#include <set>

namespace HubPlatform {
namespace Vidoo4Hub {

class IVidooManagerResources;
class IDictionaryManager;

class VidooManager : public IVidooManager {
public:
    VidooManager( IVidooManagerResources& resources, IDictionaryManager& dictionaryManager );

    bool updateObjectList( const std::vector< Json >& objectList ) override;
    bool updatePeopleList( const std::vector< Json >& peopleList ) override;
    bool updateQRCodeList( const std::vector< Json >& qrCodeList ) override;
    bool updateHotzoneList( const std::vector< Json >& hotzoneList ) override;

    void motion( const std::string& cameraId,
                 const std::string& hotzoneId,
                 const DetectedObjects& detectedObjects ) override;

    void syncUpdatedHotzones() override;
    void syncUpdatedDetectableObjects() override;

private:
    IVidooManagerResources& m_resources;
    IDictionaryManager& m_dictionaryManager;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
