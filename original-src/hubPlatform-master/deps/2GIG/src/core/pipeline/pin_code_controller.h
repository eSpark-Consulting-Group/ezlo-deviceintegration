#pragma once

#include "utils/pipeline/pipe_consumer.h"
#include "utils/pipeline/pipe_producer.h"

#include "core/pipeline/helpers/pin_code_buffer.h"
#include "core/packets/pin_code_transmission.h"

#include "2gig/device_status.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace TwoGig {

class DevicesRepository;
struct Transmission;

class PinCodeController : public PipeConsumer< Transmission >, public PipeProducer< PinCodeTransmission > {
public:
    PinCodeController( const Logger& logger, const DevicesRepository& devices );

    void processPayload( Transmission packet ) override;

    void reset() override;

private:
    char getKeypadInput( Transmission packet ) const;

    void processInput( uint32_t txId, char input );

    PinCodeBuffer& getBuffer( uint32_t txId );

private:
    const DevicesRepository& m_devices;

    std::mutex m_mutex;

    std::unordered_map< uint32_t, std::unique_ptr< PinCodeBuffer > > m_buffers;
};

}  // namespace TwoGig
