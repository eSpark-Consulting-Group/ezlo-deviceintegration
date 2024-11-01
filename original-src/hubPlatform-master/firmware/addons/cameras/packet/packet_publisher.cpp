#include "cameras/packet/packet_publisher.h"

namespace HubPlatform {
namespace Cameras {

void PacketPublisher::sendPacket( const Packet& packet ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    for( const auto& listener : m_listeners ) {
        listener.second( packet );
    }
}

IPacketPublisher::ListenerId PacketPublisher::addListener( PacketHandlerFunc func ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    ListenerId id = m_counter++;
    m_listeners.emplace( id, std::move( func ) );
    return id;
}

void PacketPublisher::removeListener( const ListenerId& id ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    m_listeners.erase( id );
}

}  // namespace Cameras

void PacketHandler::processPacket( const AVPacketPrototype& packet ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    for( const auto& context : m_listeners ) {
        if( context.streamIndex != -1 && ( packet.getPacket().stream_index != context.streamIndex ) ) {
            continue;
        }
        context.func( packet.clone() );
    }
}

PacketHandler::ListenerId PacketHandler::addListener( PacketHandler::PacketHandlerFunc func, int streamIndex ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    ListenerId id = m_counter++;
    m_listeners.emplace_back( id, std::move( func ), streamIndex );
    return id;
}

void PacketHandler::removeListener( const PacketHandler::ListenerId& id ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    for( auto it = m_listeners.begin(); it != m_listeners.end(); ++it ) {
        if( it->id == id ) {
            m_listeners.erase( it );
            return;
        }
    }
}

PacketHandler::ListenerContext::ListenerContext( PacketHandler::ListenerId _id,
                                                 PacketHandler::PacketHandlerFunc _func,
                                                 int _streamIndex )
: id( _id )
, streamIndex( _streamIndex )
, func( std::move( _func ) ) {}

PacketListenerRemover::PacketListenerRemover( PacketHandler& frameHandler,
                                              PacketHandler::PacketHandlerFunc&& func,
                                              int streamIndex )
: m_packetHandler( frameHandler ) {
    m_listenerId = m_packetHandler.addListener( std::move( func ), streamIndex );
}

PacketListenerRemover::~PacketListenerRemover() {
    m_packetHandler.removeListener( m_listenerId );
    if( m_actionAfterRemoving ) {
        m_actionAfterRemoving();
    }
}

void PacketListenerRemover::setActionAfterListenerRemoving( const std::function< void() >& action ) {
    m_actionAfterRemoving = action;
}

}  // namespace HubPlatform
