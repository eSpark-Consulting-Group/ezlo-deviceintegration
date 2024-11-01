#pragma once

#include "cameras/events/camera_base_event.h"

namespace HubPlatform {
namespace Cameras {

template < typename EventType >
class IEventPublisher {
public:
    using EventHandler = std::function< void( const EventType& ) >;
    using ListenerId = int64_t;

    virtual ~IEventPublisher() = default;
    virtual void sendEvent( EventType ) = 0;
    virtual ListenerId addListener( EventHandler func ) = 0;
    virtual void removeListener( const ListenerId& id ) = 0;
};

}  // namespace Cameras
}  // namespace HubPlatform
