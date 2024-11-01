#include "cameras/packet/packets_buffer.h"

namespace HubPlatform {

PacketsBuffer::PacketsBuffer( size_t frameCountLimit, SkippingType skippingType )
: m_frameCountLimit( frameCountLimit )
, m_skippingType( skippingType ) {}

bool PacketsBuffer::appendElement( const Packet& packet ) {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    if( ( m_frameCountLimit != 0 ) && ( m_queue.size() >= m_frameCountLimit ) ) {
        if( m_skippingType == SkippingType::Remove ) {
            clearQueue();
        }
        return false;
    }
    m_queue.emplace( packet.clone() );
    return true;
}

bool PacketsBuffer::getFrontElement( Packet& outPacket ) {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    if( !m_queue.empty() ) {
        outPacket = std::move( m_queue.front() );
        m_queue.pop();
        return true;
    }
    return false;
}

bool PacketsBuffer::empty() const {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    return m_queue.empty();
}

void PacketsBuffer::clear() {
    std::lock_guard< std::mutex > guard( m_queueMutex );
    clearQueue();
}

void PacketsBuffer::clearQueue() {
    while( !m_queue.empty() ) {
        m_queue.pop();
    }
}

}  // namespace HubPlatform
