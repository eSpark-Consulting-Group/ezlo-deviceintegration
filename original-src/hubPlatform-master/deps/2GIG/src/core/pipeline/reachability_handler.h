#pragma once

#include "2gig/device_status.h"

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

namespace TwoGig {

struct Transmission;
struct DevicesReachabilityManager;
class Logger;

class ReachabilityHandler : public PipeConsumer< Transmission >, public PipeProducer< Transmission, 1 > {
public:
    ReachabilityHandler( const Logger& logger, DevicesReachabilityManager& reachabilityManager );

    void processPayload( Transmission packet ) override;

private:
    DevicesReachabilityManager& m_reachabilityManager;
};

}  // namespace TwoGig
