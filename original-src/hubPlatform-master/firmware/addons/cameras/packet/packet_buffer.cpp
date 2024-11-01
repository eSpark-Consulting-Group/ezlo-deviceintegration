#include "cameras/packet/packet_buffer.h"
#include <thread>

namespace HubPlatform {
namespace Cameras {

PacketBuffer::PacketBuffer( size_t frameCountLimit, SkippingType skippingType )
: m_packetCountLimit( frameCountLimit )
, m_skippingType( skippingType ) {}

bool PacketBuffer::push( const Packet& packet ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    if( ( m_packetCountLimit != 0 ) && ( m_queue.size() >= m_packetCountLimit ) ) {
        if( m_skippingType == SkippingType::Remove ) {
            m_queue.clear();
        }
        return false;
    }
    m_queue.push_back( packet );
    return true;
}

bool PacketBuffer::waitFor( std::chrono::seconds timeout ) const {
    if( empty() ) {
        std::this_thread::sleep_for( timeout );
        return false;
    }
    return true;
}

bool PacketBuffer::pop( Packet& packet ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    if( !m_queue.empty() ) {
        packet = std::move( m_queue.front() );
        m_queue.pop_front();
        return true;
    }
    return false;
}

bool PacketBuffer::empty() const {
    std::lock_guard< std::mutex > guard( m_mutex );
    return m_queue.empty();
}

void PacketBuffer::clear() {
    std::lock_guard< std::mutex > guard( m_mutex );
    m_queue.clear();
}

}  // namespace Cameras
}  // namespace HubPlatform
