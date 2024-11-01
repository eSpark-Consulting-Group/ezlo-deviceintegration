#include "cameras/frame/frames_buffer.h"

namespace HubPlatform {

bool FramesBuffer::appendElement( const Frame& frame ) {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    m_frame = frame;
    return true;
}

bool FramesBuffer::getFrontElement( Frame& frame ) {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    if( m_frame ) {
        frame = std::move( m_frame );
        return true;
    }
    return false;
}

bool FramesBuffer::empty() const {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    return static_cast< bool >( m_frame );
}

void FramesBuffer::clear() {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    if( m_frame ) {
        m_frame.reset();
    }
}

}  // namespace HubPlatform
