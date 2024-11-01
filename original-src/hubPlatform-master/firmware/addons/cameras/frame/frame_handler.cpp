#include "cameras/frame/frame_handler.h"

namespace HubPlatform {

void FrameHandler::processFrame( const FrameWrapperSharedPtr& frame ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    for( const auto& context : m_listeners ) {
        context.second( frame );
    }
}

FrameHandler::ListenerId FrameHandler::addListener( FrameHandler::FrameHandlerFunc func ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    ListenerId id = m_counter++;
    m_listeners[ id ] = std::move( func );
    return id;
}

void FrameHandler::removeListener( const FrameHandler::ListenerId& id ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    m_listeners.erase( id );
}

}  // namespace HubPlatform
