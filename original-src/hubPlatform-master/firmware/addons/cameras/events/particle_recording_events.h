#pragma once
#include "cameras/events/camera_base_event.h"

namespace HubPlatform {

class MakingVideoParticleFailed : public CameraBaseEvent {
public:
    explicit MakingVideoParticleFailed( const std::string& streamId, const std::string& errorMessage );
};

}  // namespace HubPlatform
