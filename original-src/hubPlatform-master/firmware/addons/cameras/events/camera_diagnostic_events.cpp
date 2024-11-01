#include "cameras/events/camera_diagnostic_events.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

CameraAccumulatedDelay::CameraAccumulatedDelay( const std::string& streamId )
: CameraBaseEvent( "camera_accumulated_delay", JSON( "streamId" << streamId ) ) {}

CameraPacketDelay::CameraPacketDelay( const std::string& streamId )
: CameraBaseEvent( "camera_packet_delay", JSON( "streamId" << streamId ) ) {}

CameraStreamClosed::CameraStreamClosed( const std::string& streamId )
: CameraBaseEvent( "camera_stream_closed", JSON( "streamId" << streamId ) ) {}

CameraUnreachable::CameraUnreachable( const std::string& streamId )
: CameraBaseEvent( "camera_unreachable", JSON( "streamId" << streamId ) ) {}

CloudTunnelRejected::CloudTunnelRejected( const std::string& streamId )
: CameraBaseEvent( "cloud_tunnel_rejected", JSON( "streamId" << streamId ) ) {}

CloudTunnelEstablished::CloudTunnelEstablished( const std::string& streamId )
: CameraBaseEvent( "cloud_tunnel_established", JSON( "streamId" << streamId ) ) {}

LocalNetworkConnected::LocalNetworkConnected()
: CameraBaseEvent( "hub_connected_to_local_network", Json() ) {}

LocalNetworkDisconnected::LocalNetworkDisconnected()
: CameraBaseEvent( "hub_disconnected_from_local_network", Json() ) {}

}  // namespace HubPlatform
