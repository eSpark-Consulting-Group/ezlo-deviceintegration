#pragma once

#include "core/packets/packets.h"

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

namespace TwoGig {

class PacketValidator : public PipeConsumer< PacketBytes >, public PipeProducer< PacketBytes > {
public:
    PacketValidator( const Logger& logger );

    void processPayload( PacketBytes packet ) override;

private:
    bool validatePacket( const PacketBytes& packet ) const;
};

}  // namespace TwoGig
