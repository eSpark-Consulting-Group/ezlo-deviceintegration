#include "cameras/events/particle_recording_events.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

MakingVideoParticleFailed::MakingVideoParticleFailed( const std::string& streamId, const std::string& errorMessage )
: CameraBaseEvent( "making_video_particle_failed", JSON( "streamId" << streamId << "errorMessage" << errorMessage ) ) {}

}  // namespace HubPlatform
