#include "core/pipeline/device_filter.h"
#include "core/packets/packets.h"

#include "utils/logger/logger.h"

namespace TwoGig {

DeviceFilter::DeviceFilter( const Logger& logger, const DevicesRepository& devices )
: PipeProducer< Transmission, 2 >( logger )
, m_devices( devices ) {}

void DeviceFilter::processPayload( Transmission packet ) {
    dispatchPayload( packet,
                     m_devices.hasDevice( packet.serial ) ? PipeChannel::KnownDevice : PipeChannel::UnknownDevice );
}

}  // namespace TwoGig
