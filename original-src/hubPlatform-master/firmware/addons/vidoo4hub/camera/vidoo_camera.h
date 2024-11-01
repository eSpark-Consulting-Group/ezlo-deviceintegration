#pragma once

#include <string>

namespace HubPlatform {
namespace Vidoo4Hub {

struct VidooCamera {
    std::string cameraId;
    std::string cameraSubcategory;
    std::string cameraName;
    std::string cameraDeviceId;

    std::string sensorDeviceId;
    std::string securityThreatItemId;
    std::string motionItemId;

    std::string hotzonesItemId;

    std::string snapshotItemId;
    std::string makeRecordingItemId;
    std::string stopRecordingItemId;

    std::string rtspURL;
    std::string streamId;

    bool operator==( const VidooCamera& camera ) const;
    bool operator!=( const VidooCamera& camera ) const;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
