#pragma once
#include "cameras/packet/packet.h"
#include "cameras/packet/packet_buffer_interface.h"

#include <functional>
#include <map>
#include <mutex>

namespace HubPlatform {
namespace Cameras {

class IPacketPublisher {
public:
    using ListenerId = int64_t;
    using PacketHandlerFunc = std::function< void( const Packet& ) >;

    virtual ~IPacketPublisher() = default;
    virtual void sendPacket( const Packet& packet ) = 0;
    virtual ListenerId addListener( PacketHandlerFunc func ) = 0;
    virtual void removeListener( const ListenerId& id ) = 0;
};

}  // namespace Cameras
}  // namespace HubPlatform
