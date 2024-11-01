#pragma once

#include "cameras/packet/packet.h"

#include <queue>
#include <mutex>

namespace HubPlatform {

class PacketsBuffer {
public:
    using Packet = AVPacketPrototype;

    enum class SkippingType { Keep, Remove };

    ///
    /// \brief PacketsBuffer
    /// \param frameCountLimit zero means unlimited
    ///
    explicit PacketsBuffer( size_t frameCountLimit = 0, SkippingType skippingType = SkippingType::Remove );
    PacketsBuffer( const PacketsBuffer& ) = delete;
    PacketsBuffer( PacketsBuffer&& ) = delete;

    bool appendElement( const Packet& packet );
    bool getFrontElement( Packet& outPacket );
    bool empty() const;
    void clear();

private:
    void clearQueue();

    size_t m_frameCountLimit;
    mutable std::mutex m_queueMutex;
    std::queue< Packet > m_queue;
    const SkippingType m_skippingType;
};

}  // namespace HubPlatform
