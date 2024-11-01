#pragma once
#include "cameras/events/camera_base_event.h"

#include <string>

namespace HubPlatform {

class LiveRtspStreamStarted : public CameraBaseEvent {
public:
    LiveRtspStreamStarted( const std::string& streamId, const std::string& localRtspUrl );
};

class LiveRtspStreamStopped : public CameraBaseEvent {
public:
    explicit LiveRtspStreamStopped( const std::string& streamId );
};

class LiveRtspStreamError : public CameraBaseEvent {
public:
    LiveRtspStreamError( const std::string& streamId, const std::string& errorMessage );
    explicit LiveRtspStreamError( const std::string& errorMessage );
};

class LiveRtspStreamUpdated : public CameraBaseEvent {
public:
    LiveRtspStreamUpdated( const std::string& streamId, const std::string& localRtspUrl );
};

}  // namespace HubPlatform
