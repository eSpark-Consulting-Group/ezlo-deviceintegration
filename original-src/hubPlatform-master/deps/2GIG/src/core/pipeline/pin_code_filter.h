#pragma once

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

#include "2gig/device_status.h"

namespace TwoGig {

struct Transmission;

class PinCodeFilter : public PipeConsumer< Transmission >, public PipeProducer< Transmission, 2 > {
public:
    enum PipeChannel { StatusUpdate, PinCodeInput };

public:
    PinCodeFilter( const Logger& logger );

    void processPayload( Transmission packet ) override;

private:
    bool isPinCodeInput( Transmission packet );
};

}  // namespace TwoGig
