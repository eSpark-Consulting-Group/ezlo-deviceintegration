#pragma once

#include "2gig/devices_repository.h"

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"
#include "utils/thread/thread_safe_wrapper.h"

#include <memory>

namespace TwoGig {

struct Transmission;

class DeviceFilter : public PipeConsumer< Transmission >, public PipeProducer< Transmission, 2 > {
public:
    enum PipeChannel { KnownDevice, UnknownDevice };

public:
    DeviceFilter( const Logger& logger, const DevicesRepository& devices );

    void processPayload( Transmission packet ) override;

private:
    const DevicesRepository& m_devices;
};

}  // namespace TwoGig
