#include "core/pipeline/reachability_handler.h"
#include "core/packets/packets.h"

#include "2gig/devices_reachability_manager.h"

namespace TwoGig {

ReachabilityHandler::ReachabilityHandler( const Logger& logger, DevicesReachabilityManager& reachabilityManager )
: PipeProducer< Transmission, 1 >( logger )
, m_reachabilityManager( reachabilityManager ) {}

void ReachabilityHandler::processPayload( Transmission packet ) {
    m_reachabilityManager.onDeviceReceivedData( packet.serial );
    dispatchPayload( packet );
}

}  // namespace TwoGig
