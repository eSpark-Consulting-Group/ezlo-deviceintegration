#pragma once
#include "cameras/packet/packet.h"
#include "cameras/defs.h"

#include <functional>
#include <list>
#include <map>
#include <mutex>

namespace HubPlatform {

class FrameHandler final {
public:
    using ListenerId = int64_t;
    using FrameHandlerFunc = std::function< void( const FrameWrapperSharedPtr ) >;

    FrameHandler() = default;
    FrameHandler( const FrameHandler& ) = delete;
    FrameHandler( FrameHandler&& ) = delete;

    void processFrame( const FrameWrapperSharedPtr& frame );
    ListenerId addListener( FrameHandlerFunc func );
    void removeListener( const ListenerId& id );

private:
    ListenerId m_counter = 0;
    std::map< ListenerId, FrameHandlerFunc > m_listeners;
    std::mutex m_mutex;
};

class FrameListenerLifetimeController {
public:
    FrameListenerLifetimeController( FrameHandler& frameHandler, FrameHandler::FrameHandlerFunc&& func )
    : m_frameHandler( frameHandler ) {
        m_listenerId = m_frameHandler.addListener( std::move( func ) );
    }
    ~FrameListenerLifetimeController() {
        m_frameHandler.removeListener( m_listenerId );
    }

private:
    FrameHandler& m_frameHandler;
    FrameHandler::ListenerId m_listenerId;
};

}  // namespace HubPlatform
