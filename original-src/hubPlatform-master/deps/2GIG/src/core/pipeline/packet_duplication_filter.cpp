#include "core/pipeline/packet_duplication_filter.h"
#include "core/packets/packets.h"

#include "2gig/device_type.h"
#include "2gig/device_status.h"

#include "utils/timer/timer.h"
#include "utils/logger/logger.h"

namespace TwoGig {

PacketDuplicationFilter::PacketDuplicationFilter( const Logger& logger, const Timer& timer )
: PipeProducer< Transmission >( logger )
, m_timer( timer ) {}

void PacketDuplicationFilter::processPayload( Transmission packet ) {
    auto timestamp = m_timer.timestamp();

    const bool allowPacket = shouldAllowTransmission( packet, timestamp );

    updateTransmissionInfo( packet, timestamp );

    if( allowPacket )
        dispatchPayload( packet );
    else
        m_logger.debug( "Discarded repeating packet" );
}

void PacketDuplicationFilter::reset() {
    m_transmissionInfos.clear();
}

bool PacketDuplicationFilter::shouldAllowTransmission( const Transmission& packet, Timer::TimePoint timestamp ) const {
    auto lastTransmissionInfo = getLastTransmissionInfo( packet.serial );

    if( !lastTransmissionInfo )
        return true;

    // Note: Keypads store sequence counter in the device type field - use it instead of timestamps
    if( packet.getDeviceType() == DeviceType::Keypad )
        return lastTransmissionInfo->sequenceCounter != packet.deviceType;

    if( lastTransmissionInfo->status != packet.status )
        return true;

    if( ( timestamp - lastTransmissionInfo->timestamp ) >= getDuplicationInterval( packet ) )
        return true;

    return false;
}

const PacketDuplicationFilter::TransmissionInfo* PacketDuplicationFilter::getLastTransmissionInfo(
    uint32_t serial ) const {
    auto it = m_transmissionInfos.find( serial );

    return it == m_transmissionInfos.end() ? nullptr : &it->second;
}

void PacketDuplicationFilter::updateTransmissionInfo( const Transmission& packet, Timer::TimePoint timestamp ) {
    m_transmissionInfos[ packet.serial ] = { timestamp, packet.status, packet.deviceType };
}

Timer::Duration PacketDuplicationFilter::getDuplicationInterval( const Transmission& packet ) {
    // Todo: get interval values from a config file

    switch( packet.getDeviceType() ) {
    case DeviceType::RemoteKeyFob: {
        const auto status = packet.getStatus< RemoteKeyFobStatus >();

        // Note: These 3 buttons are activated by being held, unlike the 4th one.
        const bool buttonHeld = status.button1Pressed || status.button2Pressed || status.button3Pressed;

        return std::chrono::milliseconds( buttonHeld ? 800 : 100 );
    }

    case DeviceType::PanicButton:
    case DeviceType::DoorWindowSensor:
        return std::chrono::milliseconds( 2000 );

    case DeviceType::SmokeDetector:
        return std::chrono::milliseconds( 100 );

    case DeviceType::GlassBreakDetector:
    case DeviceType::CoDetector:
        return std::chrono::milliseconds( 200 );

    // Todo: check devices
    default:
        return std::chrono::milliseconds( 200 );
    }
}

}  // namespace TwoGig
