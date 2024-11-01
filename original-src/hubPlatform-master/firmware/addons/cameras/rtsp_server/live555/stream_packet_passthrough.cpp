#include "cameras/rtsp_server/live555/stream_packet_passthrough.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

StreamPacketPassthrough::StreamPacketPassthrough( IPacketPublisher& packetPublisher )
: m_listenersCounter( 0 )
, m_packetPublisher( packetPublisher ) {
    auto callback = std::bind( &StreamPacketPassthrough::receivePacket, this, std::placeholders::_1 );
    m_id = m_packetPublisher.addListener( std::move( callback ) );
}

StreamPacketPassthrough::~StreamPacketPassthrough() {
    m_packetPublisher.removeListener( m_id );
}

void StreamPacketPassthrough::receivePacket( const Packet& packet ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    Packet preCachedPacket = packet;
    preCachedPacket.isPreCached = true;

    if( preCachedPacket.isKey ) {
        m_cachedPackets.clear();
        m_cachedPackets.emplace_back( std::move( preCachedPacket ) );
    }
    else if( !m_cachedPackets.empty() ) {
        m_cachedPackets.emplace_back( std::move( preCachedPacket ) );
    }

    for( const auto& listenerIdHandlerPair : m_listenersData ) {
        const PacketHandlerFunc& handler = listenerIdHandlerPair.second;
        handler( packet );
    }
}

StreamPacketPassthrough::ListenerId StreamPacketPassthrough::addListener( PacketHandlerFunc packetHandler ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    for( const Packet& packet : m_cachedPackets ) {
        packetHandler( packet );
    }

    const ListenerId id = m_listenersCounter++;
    m_listenersData.emplace( id, std::move( packetHandler ) );
    return id;
}

void StreamPacketPassthrough::removeListener( ListenerId listenerId ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_listenersData.erase( listenerId );
}

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
