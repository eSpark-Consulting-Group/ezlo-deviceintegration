#pragma once
#include "cameras/packet/packet.h"
#include "cameras/packet/packet_buffer_interface.h"
#include "cameras/packet/packet_publisher_interface.h"

#include <functional>
#include <list>
#include <mutex>
#include <map>

namespace HubPlatform {
namespace Cameras {

class PacketPublisher : public IPacketPublisher {
public:
    PacketPublisher() = default;
    PacketPublisher( const PacketPublisher& ) = delete;
    PacketPublisher( PacketPublisher&& ) = delete;

    void sendPacket( const Packet& packet ) override;
    ListenerId addListener( PacketHandlerFunc func ) override;
    void removeListener( const ListenerId& id ) override;

private:
    std::mutex m_mutex;  // @todo: use shared mutex
    ListenerId m_counter = 0;
    std::map< ListenerId, PacketHandlerFunc > m_listeners;
};

}  // namespace Cameras

///
/// \brief The PacketHandler class
/// Clones incoming packet for each listener
class PacketHandler final {
public:
    PacketHandler() = default;
    PacketHandler( const PacketHandler& ) = delete;
    PacketHandler( PacketHandler&& ) = delete;

    ///
    /// \brief processPacket
    /// \param packet
    /// Called when packet received from camera reader
    void processPacket( const AVPacketPrototype& packet );

    using PacketHandlerFunc = std::function< void( const AVPacketPrototype& ) >;
    using ListenerId = int64_t;
    static constexpr ListenerId UnregisteredListenerId = -1;

    ListenerId addListener( PacketHandlerFunc func, int streamIndex = -1 );
    void removeListener( const ListenerId& id );

private:
    std::mutex m_mutex;  // @todo: use shared mutex
    ListenerId m_counter = 0;

    struct ListenerContext {
        ListenerContext( ListenerId _id, PacketHandlerFunc _func, int _streamIndex );
        ListenerContext( const ListenerContext& ) = delete;

        ListenerId id;
        int streamIndex;
        PacketHandlerFunc func;
    };

    std::list< ListenerContext > m_listeners;
};

class PacketListenerRemover {
public:
    PacketListenerRemover( PacketHandler& frameHandler, PacketHandler::PacketHandlerFunc&& func, int streamIndex = -1 );
    ~PacketListenerRemover();

    void setActionAfterListenerRemoving( const std::function< void() >& action );

private:
    PacketHandler& m_packetHandler;
    PacketHandler::ListenerId m_listenerId;
    std::function< void() > m_actionAfterRemoving;
};

}  // namespace HubPlatform
