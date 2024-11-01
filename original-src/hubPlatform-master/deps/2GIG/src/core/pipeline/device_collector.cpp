#include "core/pipeline/device_collector.h"
#include "core/packets/packets.h"

#include "2gig/client.h"

#include "utils/logger/logger.h"

namespace TwoGig {

DeviceCollector::DeviceCollector( const Logger& logger, Client& client )
: m_logger( logger )
, m_client( client ) {}

void DeviceCollector::processPayload( Transmission packet ) {
    if( m_uniqueIds.insert( packet.serial ).second ) {
        m_logger.info( "New device found..." );
        m_client.onNewDeviceDiscovered( packet.serial, packet.getDeviceType() );
    }
}

void DeviceCollector::reset() {
    m_uniqueIds.clear();
}

}  // namespace TwoGig
