#pragma once

#include <string>

namespace HubPlatform {

class ILiveStreamer {
public:
    using LocalRtspUrl = std::string;

    virtual void createCameraStream() = 0;
    virtual void deleteCameraStream() = 0;

    virtual ~ILiveStreamer() = default;
};

}  // namespace HubPlatform
