#pragma once

#include "core/packets/packets.h"

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

#include <functional>

namespace TwoGig {

class PacketCollector : public PipeConsumer< uint8_t >, public PipeProducer< PacketBytes > {
    enum class PacketState { Invalid, Incomplete, Ready };

public:
    PacketCollector( const Logger& logger );

    void processPayload( uint8_t byte ) override;

    void reset() override;

private:
    PacketState getPacketState() const;

    static PacketState sizeToState( size_t actualSize, size_t expectedSize );

private:
    std::vector< uint8_t > m_buffer;
};

}  // namespace TwoGig
