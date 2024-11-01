#pragma once

#include "vidoo4hub/detectable_types/detectable_types.h"

#include "json_utils/json_fwd.h"

#include <string>
#include <vector>
#include <set>

namespace HubPlatform {
namespace Vidoo4Hub {

class IVidooManager {
public:
    using ObjectId = std::string;
    using DetectedObjects = std::map< DetectableTypes::Type, std::set< ObjectId > >;

    virtual ~IVidooManager() = default;

    virtual bool updateObjectList( const std::vector< Json >& objectList ) = 0;
    virtual bool updatePeopleList( const std::vector< Json >& peopleList ) = 0;
    virtual bool updateQRCodeList( const std::vector< Json >& qrCodeList ) = 0;
    virtual bool updateHotzoneList( const std::vector< Json >& hotzoneList ) = 0;

    virtual void motion( const std::string& cameraId,
                         const std::string& hotzoneId,
                         const DetectedObjects& detectedObjects ) = 0;

    virtual void syncUpdatedHotzones() = 0;
    virtual void syncUpdatedDetectableObjects() = 0;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
