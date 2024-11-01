#pragma once

#include "cameras/packet/packet_publisher_interface.h"
#include "cameras/defs.h"

#include <unordered_map>
#include <functional>
#include <vector>
#include <mutex>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

class StreamPacketPassthrough {
public:
    using PacketHandlerFunc = std::function< void( const Packet& ) >;
    using ListenerId = int64_t;

public:
    StreamPacketPassthrough( IPacketPublisher& packetPublisher );
    ~StreamPacketPassthrough();

    void receivePacket( const Packet& packet );

    ListenerId addListener( PacketHandlerFunc packetHandler );
    void removeListener( ListenerId listenerId );

private:
    using CachedPackets = std::vector< Packet >;
    using ListenersData = std::unordered_map< ListenerId, PacketHandlerFunc >;

private:
    std::mutex m_mutex;
    CachedPackets m_cachedPackets;
    ListenerId m_listenersCounter;
    ListenersData m_listenersData;

    IPacketPublisher& m_packetPublisher;
    IPacketPublisher::ListenerId m_id;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
