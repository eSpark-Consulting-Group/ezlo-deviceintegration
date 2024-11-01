#include "core/pipeline/packet_validator.h"
#include "core/packets/packets.h"
#include "core/packets/packet_type.h"

#include "utils/crc/crc.h"
#include "utils/logger/logger.h"

namespace TwoGig {

PacketValidator::PacketValidator( const Logger& logger )
: PipeProducer< PacketBytes >( logger ) {}

void PacketValidator::processPayload( PacketBytes packet ) {
    if( validatePacket( packet ) )
        dispatchPayload( std::move( packet ) );
    else
        m_logger.warn( "Discarded invalid packet" );
}

bool PacketValidator::validatePacket( const PacketBytes& packet ) const {
    if( packet[ 0 ] != PacketType::Regular )
        return true;

    // first byte does not participate in the computation
    return computeCRC8( packet.data() + 1, packet.size() - 1 ) == 0;
}

}  // namespace TwoGig
