#pragma once

#include "2gig/device_type.h"

#include "utils/pipeline/pipe_consumer.h"

#include <set>
#include <vector>
#include <cstdint>

namespace TwoGig {

struct Transmission;
struct Client;
class Logger;

class DeviceCollector : public PipeConsumer< Transmission > {
public:
    DeviceCollector( const Logger& logger, Client& client );

    void processPayload( Transmission packet ) override;

    void reset() override;

private:
    const Logger& m_logger;
    Client& m_client;

    std::set< uint32_t > m_uniqueIds;
};

}  // namespace TwoGig
