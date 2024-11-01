#pragma once

#include "cameras/packet/packet_buffer_interface.h"

#include <mutex>
#include <deque>

namespace HubPlatform {
namespace Cameras {

class PacketBuffer : public IPacketBuffer {
public:
    enum class SkippingType { Keep, Remove };

    ///
    /// \brief PacketsBuffer
    /// \param packetCountLimit zero means unlimited
    /// \param skippingType The remove type removes queue
    ///
    explicit PacketBuffer( size_t packetCountLimit = 0, SkippingType skippingType = SkippingType::Remove );
    PacketBuffer( const PacketBuffer& ) = delete;
    PacketBuffer( PacketBuffer&& ) = delete;

    bool push( const Packet& ) override;
    bool waitFor( std::chrono::seconds ) const override;
    bool pop( Packet& ) override;
    bool empty() const override;
    void clear() override;

private:
    size_t m_packetCountLimit;
    mutable std::mutex m_mutex;
    std::deque< Packet > m_queue;
    const SkippingType m_skippingType;
};

}  // namespace Cameras
}  // namespace HubPlatform
