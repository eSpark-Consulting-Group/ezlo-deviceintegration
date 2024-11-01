#include "cameras/events/live_rtsp_stream.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

LiveRtspStreamStarted::LiveRtspStreamStarted( const std::string& streamId, const std::string& localRtspUrl )
: CameraBaseEvent( "live_rtsp_stream_started", JSON( "streamId" << streamId << "localRtspUrl" << localRtspUrl ) ) {}

LiveRtspStreamStopped::LiveRtspStreamStopped( const std::string& streamId )
: CameraBaseEvent( "live_rtsp_stream_stopped", JSON( "streamId" << streamId ) ) {}

LiveRtspStreamError::LiveRtspStreamError( const std::string& streamId, const std::string& errorMessage )
: CameraBaseEvent( "live_rtsp_stream_error", JSON( "streamId" << streamId << "errorMessage" << errorMessage ) ) {}

LiveRtspStreamError::LiveRtspStreamError( const std::string& errorMessage )
: CameraBaseEvent( "live_rtsp_stream_error", JSON( "errorMessage" << errorMessage ) ) {}

LiveRtspStreamUpdated::LiveRtspStreamUpdated( const std::string& streamId, const std::string& localRtspUrl )
: CameraBaseEvent( "live_rtsp_stream_updated", JSON( "streamId" << streamId << "localRtspUrl" << localRtspUrl ) ) {}

}  // namespace HubPlatform
