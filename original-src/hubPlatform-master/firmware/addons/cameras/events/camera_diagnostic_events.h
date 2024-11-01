#pragma once

#include "cameras/events/camera_base_event.h"
#include <string>

namespace HubPlatform {

class CameraAccumulatedDelay : public CameraBaseEvent {
public:
    explicit CameraAccumulatedDelay( const std::string& streamId );
};

class CameraPacketDelay : public CameraBaseEvent {
public:
    explicit CameraPacketDelay( const std::string& streamId );
};

class CameraStreamClosed : public CameraBaseEvent {
public:
    explicit CameraStreamClosed( const std::string& streamId );
};

class CameraUnreachable : public CameraBaseEvent {
public:
    explicit CameraUnreachable( const std::string& streamId );
};

class CloudTunnelRejected : public CameraBaseEvent {
public:
    explicit CloudTunnelRejected( const std::string& streamId );
};

class CloudTunnelEstablished : public CameraBaseEvent {
public:
    explicit CloudTunnelEstablished( const std::string& streamId );
};

class LocalNetworkConnected : public CameraBaseEvent {
public:
    LocalNetworkConnected();
};

class LocalNetworkDisconnected : public CameraBaseEvent {
public:
    LocalNetworkDisconnected();
};

}  // namespace HubPlatform
