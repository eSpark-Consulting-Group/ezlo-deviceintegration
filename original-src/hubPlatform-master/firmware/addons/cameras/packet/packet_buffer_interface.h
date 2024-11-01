#pragma once

#include "cameras/packet/packet.h"

namespace HubPlatform {
namespace Cameras {

class IPacketBuffer {
public:
    virtual ~IPacketBuffer() = default;
    virtual bool push( const Packet& ) = 0;
    virtual bool waitFor( std::chrono::seconds ) const = 0;
    virtual bool pop( Packet& ) = 0;
    virtual bool empty() const = 0;
    virtual void clear() = 0;
};

}  // namespace Cameras
}  // namespace HubPlatform
