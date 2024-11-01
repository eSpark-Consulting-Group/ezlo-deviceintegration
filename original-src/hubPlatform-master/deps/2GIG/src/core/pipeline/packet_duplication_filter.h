#pragma once

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

#include "utils/timer/timer.h"

#include <map>

namespace TwoGig {

struct Transmission;

class PacketDuplicationFilter : public PipeConsumer< Transmission >, public PipeProducer< Transmission > {
    struct TransmissionInfo {
        Timer::TimePoint timestamp;
        uint8_t status;
        uint8_t sequenceCounter;
    };

public:
    PacketDuplicationFilter( const Logger& logger, const Timer& timer );

    void processPayload( Transmission packet ) override;

    void reset() override;

private:
    bool shouldAllowTransmission( const Transmission& packet, Timer::TimePoint timestamp ) const;

    const TransmissionInfo* getLastTransmissionInfo( uint32_t serial ) const;

    void updateTransmissionInfo( const Transmission& packet, Timer::TimePoint timestamp );

    static Timer::Duration getDuplicationInterval( const Transmission& packet );

private:
    const Timer& m_timer;

    std::map< uint32_t, TransmissionInfo > m_transmissionInfos;
};

}  // namespace TwoGig
