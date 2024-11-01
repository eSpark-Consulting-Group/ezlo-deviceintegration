#include "core/pipeline/transmission_handler.h"
#include "core/packets/packets.h"

#include "2gig/client.h"
#include "2gig/device_type.h"

#include "utils/logger/logger.h"

#include <sstream>
#include <iomanip>
#include <bitset>

namespace TwoGig {

TransmissionHandler::TransmissionHandler( const Logger& logger, Client& client, const DevicesRepository& devices )
: m_logger( logger )
, m_client( client )
, m_devices( devices ) {}

void TransmissionHandler::processPayload( Transmission packet ) {
    switch( getDeviceType( packet ) ) {
    case DeviceType::SmokeDetector:
        printTransmissionInfo( packet, "Dispatching smoke detector transmission..." );
        m_client.onSmokeDetectorUpdated( packet.serial, packet.getStatus< SmokeDetectorStatus >() );
        break;

    case DeviceType::PanicButton:
        printTransmissionInfo( packet, "Dispatching panic button transmission..." );
        m_client.onPanicButtonUpdated( packet.serial, packet.getStatus< PanicButtonStatus >() );
        break;

    case DeviceType::PirDetector:
        printTransmissionInfo( packet, "Dispatching PIR detector transmission..." );
        m_client.onPirDetectorUpdated( packet.serial, packet.getStatus< PirDetectorStatus >() );
        break;

    case DeviceType::FloodTemperature:
        printTransmissionInfo( packet, "Dispatching flood/temperature transmission..." );
        m_client.onFloodTemperatureUpdated( packet.serial, packet.getStatus< FloodTemperatureStatus >() );
        break;

    case DeviceType::GlassBreakDetector:
        printTransmissionInfo( packet, "Dispatching glass break detector transmission..." );
        m_client.onGlassBreakDetectorUpdated( packet.serial, packet.getStatus< GlassBreakStatus >() );
        break;

    case DeviceType::DoorWindowSensor:
        printTransmissionInfo( packet, "Dispatching door/window sensor transmission..." );
        m_client.onDoorWindowSensorUpdated( packet.serial, packet.getStatus< DoorWindowSensorStatus >() );
        break;

    case DeviceType::CoDetector:
        printTransmissionInfo( packet, "Dispatching CO detector transmission..." );
        m_client.onCoDetectorUpdated( packet.serial, packet.getStatus< CoDetectorStatus >() );
        break;

    case DeviceType::TiltDetector:
        printTransmissionInfo( packet, "Dispatching tilt detector transmission..." );
        m_client.onTiltDetectorUpdated( packet.serial, packet.getStatus< TiltDetectorStatus >() );
        break;

    case DeviceType::RemoteKeyFob:
        printTransmissionInfo( packet, "Dispatching remote keyfob transmission..." );
        m_client.onRemoteKeyFobUpdated( packet.serial, packet.getStatus< RemoteKeyFobStatus >() );
        break;

    case DeviceType::Keypad:
        printTransmissionInfo( packet, "Dispatching pad transmission..." );
        m_client.onPadUpdated( packet.serial, packet.getStatus< PadStatus >() );
        return;

    case DeviceType::HoneywellSensor:
        printTransmissionInfo( packet, "Dispatching honeywell sensor transmission..." );
        m_client.onHoneywellSensorUpdated( packet.serial, packet.getStatus< HoneywellSensorStatus >() );
        break;

    case DeviceType::Repeater:
        printTransmissionInfo( packet, "Dispatching repeater transmission..." );
        m_client.onRepeaterUpdated( packet.serial, packet.getStatus< RepeaterStatus >() );
        return;

    default:
        printTransmissionInfo( packet, "Discarding unknown transmission..." );
        break;
    }
}

void TransmissionHandler::processPayload( PinCodeTransmission packet ) {
    printTransmissionInfo( packet, "Dispatching pin transmission..." );
    m_client.onPinUpdated( packet.txId, packet.pin, packet.pinState );
}

DeviceType::Enum TransmissionHandler::getDeviceType( Transmission packet ) const {
    return m_devices.getDeviceType( packet.serial ) == DeviceType::Repeater ? DeviceType::Repeater :
                                                                              packet.getDeviceType();
}

template < typename TransmissionT >
void TransmissionHandler::printTransmissionInfo( TransmissionT&& transmission, const char* message ) {
    if( m_logger.getLogLevel() <= InstrumentationClient::LogLevel::Info ) {
        std::stringstream logMessage;

        logMessage << transmission << " / " << message;

        m_logger.info( logMessage.str() );
    }
}

std::ostream& operator<<( std::ostream& str, const Transmission& packet ) {
    str << "TXID: " << std::setw( 7 ) << std::setfill( '0' ) << packet.serial;
    str << " / status: 0b" << std::bitset< 8 >( packet.status );
    str << " / RSSI: " << std::setw( 3 ) << std::setfill( '0' ) << std::to_string( packet.signalStrength );

    return str;
}

const char* toString( PinCodeState::Enum state ) {
    switch( state ) {
    case PinCodeState::Partial:
        return "partial";
    case PinCodeState::Valid:
        return "valid";
    case PinCodeState::Timeout:
        return "timeout";
    default:
        return "";
    }
}

std::ostream& operator<<( std::ostream& str, const PinCodeTransmission& packet ) {
    str << "TXID: " << std::setw( 7 ) << std::setfill( '0' ) << packet.txId;
    str << " / pin state: " << toString( packet.pinState );
    str << " / pin: '" << packet.pin << "'";

    return str;
}

}  // namespace TwoGig
