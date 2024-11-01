#include "core/pipeline/packet_collector.h"
#include "core/packets/packet_type.h"
#include "core/packets/packet_id.h"
#include "core/packets/packets.h"

#include "utils/logger/logger.h"

#include <cassert>

namespace TwoGig {

PacketCollector::PacketCollector( const Logger& logger )
: PipeProducer< PacketBytes >( logger ) {}

void PacketCollector::processPayload( uint8_t byte ) {
    m_buffer.push_back( byte );

    switch( getPacketState() ) {
    case PacketState::Incomplete:
        break;

    case PacketState::Ready:
        if( m_logger.getLogLevel() <= InstrumentationClient::LogLevel::Debug ) {
            m_logger.debug( "Received packet: " + Logger::bytesToString( m_buffer.data(), m_buffer.size() ) );
        }

        dispatchPayload( std::move( m_buffer ) );
        m_buffer.clear();
        break;

    case PacketState::Invalid:
        m_logger.error( "Invalid collector state" );
        m_buffer.clear();
        break;
    }
}

void PacketCollector::reset() {
    m_buffer.clear();
}

PacketCollector::PacketState PacketCollector::getPacketState() const {
    const size_t bufferSize = m_buffer.size();
    assert( bufferSize != 0 );

    switch( m_buffer[ 0 ] ) {
    case PacketType::Supervisory:
        return sizeToState( bufferSize, 1 );

    case PacketType::Regular:
        if( m_buffer.size() < 8 )
            return PacketState::Incomplete;

        if( m_buffer[ 1 ] == PacketId::TwoGig11ByteTransmission )
            return sizeToState( bufferSize, 11 );
        else
            return sizeToState( bufferSize, 8 );

    default:
        return PacketState::Invalid;
    }
}

PacketCollector::PacketState PacketCollector::sizeToState( size_t actualSize, size_t expectedSize ) {
    if( actualSize == expectedSize )
        return PacketState::Ready;

    if( actualSize < expectedSize )
        return PacketState::Incomplete;

    return PacketState::Invalid;
}

}  // namespace TwoGig
