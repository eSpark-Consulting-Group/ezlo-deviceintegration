#pragma once

#include <string>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class IStreamManager {
public:
    virtual ~IStreamManager() = default;
    virtual void addCameraStream( const std::string& streamItemId,
                                  const std::string& streamName,
                                  const std::string& rtspURL ) = 0;
    virtual void removeCameraStream( const std::string& streamItemId ) = 0;
    virtual void createLiveRTSPStream( const std::string& streamItemId ) = 0;
    virtual void deleteLiveRTSPStream( const std::string& streamItemId ) = 0;
    virtual Json getCameraStreams() = 0;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
