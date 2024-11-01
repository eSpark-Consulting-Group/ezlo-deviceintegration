#pragma once

#include "core/packets/packets.h"

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

namespace TwoGig {

class PacketSorter : public PipeConsumer< PacketBytes >, public PipeProducer< PacketBytes, 7 > {
public:
    enum PipeChannel { Transmission, EncryptedTransmission, JamDetect, JamRestore, PowerOn, Supervisory, Unknown };

public:
    PacketSorter( const Logger& logger );

    void processPayload( PacketBytes packet ) override;

private:
    PipeChannel packetIdToChannel( uint8_t packetId ) const;
};

}  // namespace TwoGig
