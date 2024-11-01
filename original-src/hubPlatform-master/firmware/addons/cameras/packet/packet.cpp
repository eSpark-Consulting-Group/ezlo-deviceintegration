#include "cameras/packet/packet.h"
#include <memory>

namespace HubPlatform {

AVPacketPrototype::AVPacketPrototype()
: m_isValid( false ) {
    av_init_packet( &m_packet );
}

AVPacketPrototype::AVPacketPrototype( const AVPacket& packet )
: m_packet( packet )
, m_isValid( true ) {}

AVPacketPrototype::~AVPacketPrototype() {
    if( m_isValid )
        av_packet_unref( &m_packet );
}

AVPacketPrototype::AVPacketPrototype( AVPacketPrototype&& r ) noexcept {
    m_packet = r.m_packet;
    m_isValid = r.m_isValid;
    r.m_isValid = false;
}

AVPacketPrototype& AVPacketPrototype::operator=( AVPacketPrototype&& r ) noexcept {
    m_packet = r.m_packet;
    m_isValid = r.m_isValid;
    r.m_isValid = false;
    return *this;
}

AVPacketPrototype AVPacketPrototype::clone() const {
    AVPacketPrototype wrapper;
    if( m_isValid && ( av_packet_ref( &wrapper.m_packet, &m_packet ) == 0 ) ) {
        wrapper.m_isValid = true;
    }
    return wrapper;
}

bool AVPacketPrototype::isValid() const {
    return m_isValid;
}

AVPacket& AVPacketPrototype::getPacket() {
    return m_packet;
}

const AVPacket& AVPacketPrototype::getPacket() const {
    return m_packet;
}

AVPacketWrapper::AVPacketWrapper()
: m_packet( av_packet_alloc() ) {
    memset( m_packet, 0, sizeof( AVPacket ) );
}

AVPacketWrapper::~AVPacketWrapper() {
    av_packet_free( &m_packet );
}

AVPacketWrapper::AVPacketWrapper( AVPacketWrapper&& r ) noexcept {
    this->operator=( std::move( r ) );
}

AVPacketWrapper& AVPacketWrapper::operator=( AVPacketWrapper&& r ) noexcept {
    // Handle a = a case.
    if( this != &r ) {
        av_packet_free( &m_packet );
        m_packet = r.m_packet;
        r.m_packet = nullptr;
    }
    return *this;
}

AVPacket* AVPacketWrapper::getPacket() {
    return m_packet;
}

}  // namespace HubPlatform
