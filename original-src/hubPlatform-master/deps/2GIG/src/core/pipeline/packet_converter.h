#pragma once

#include "core/packets/packets.h"

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

namespace TwoGig {

template < typename To >
class PacketConverter : public PipeConsumer< PacketBytes >, public PipeProducer< To > {
public:
    PacketConverter( const Logger& logger );

    void processPayload( PacketBytes packet ) override;
};

}  // namespace TwoGig
