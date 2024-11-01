#include "core/pipeline/packet_sorter.h"
#include "core/packets/packets.h"
#include "core/packets/packet_id.h"
#include "core/packets/packet_type.h"

#include "utils/logger/logger.h"

namespace TwoGig {

PacketSorter::PacketSorter( const Logger& logger )
: PipeProducer< PacketBytes, 7 >( logger ) {}

void PacketSorter::processPayload( PacketBytes packet ) {
    switch( packet[ 0 ] ) {
    case PacketType::Regular:
        dispatchPayload( std::move( packet ), packetIdToChannel( packet[ 1 ] ) );
        break;

    case PacketType::Supervisory:
        dispatchPayload( std::move( packet ), PipeChannel::Supervisory );
        break;

    default:
        m_logger.warn( "Discarded unknown packet type" );
        dispatchPayload( std::move( packet ), PipeChannel::Unknown );
        break;
    }
}

PacketSorter::PipeChannel PacketSorter::packetIdToChannel( uint8_t packetId ) const {
    switch( packetId ) {
    case PacketId::TwoGig8ByteTransmission:
    case PacketId::HoneywellTransmission:
        return PipeChannel::Transmission;

    case PacketId::TwoGig11ByteTransmission:
        return PipeChannel::EncryptedTransmission;

    case PacketId::JamDetect:
        return PipeChannel::JamDetect;

    case PacketId::JamRestore:
        return PipeChannel::JamRestore;

    case PacketId::PowerOn:
        return PipeChannel::PowerOn;

    default:
        m_logger.warn( "Discarded unknown packet type" );
        return PipeChannel::Unknown;
    }
}

}  // namespace TwoGig
