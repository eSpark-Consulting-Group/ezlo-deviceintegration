#pragma once
#include "cameras/events/camera_base_event.h"

#include <string>

namespace HubPlatform {

class StreamSnapshotted : public CameraBaseEvent {
public:
    StreamSnapshotted( const std::string& streamId, const std::string& snapshotId, const Json& cloudStorageKey );
};

class StreamSnapshotError : public CameraBaseEvent {
public:
    StreamSnapshotError( const std::string& streamId, const std::string& snapshotId, const std::string& errorMessage );
    StreamSnapshotError( const std::string& streamId, const std::string& errorMessage );
    explicit StreamSnapshotError( const std::string& errorMessage );
};

}  // namespace HubPlatform
