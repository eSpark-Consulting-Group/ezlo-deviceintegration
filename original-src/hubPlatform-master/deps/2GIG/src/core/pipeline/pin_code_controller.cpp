#include "2gig/devices_repository.h"

#include "core/pipeline/pin_code_controller.h"
#include "core/packets/packets.h"

#include "utils/logger/logger.h"
#include "utils/cpp14.h"

namespace TwoGig {

PinCodeController::PinCodeController( const Logger& logger, const DevicesRepository& devices )
: PipeProducer< PinCodeTransmission >( logger )
, m_devices( devices ) {}

void PinCodeController::processPayload( Transmission packet ) {
    if( const char input = getKeypadInput( packet ) ) {
        processInput( packet.serial, input );
    }
}

char PinCodeController::getKeypadInput( Transmission packet ) const {
    if( packet.getDeviceType() != DeviceType::Keypad )
        return 0;

    const auto status = packet.getStatus< PadStatus >();
    switch( status.buttonPressed ) {
    case PadButton::Asterisk:
        return '*';
    case PadButton::Pound:
        return '#';
    case PadButton::Num_0:
        return '0';
    default:
        if( status.buttonPressed >= PadButton::Num_1 && status.buttonPressed <= PadButton::Num_9 )
            return '0' + status.buttonPressed;
        else
            return 0;
    }
}

void PinCodeController::processInput( uint32_t txId, char input ) {
    std::unique_lock< std::mutex > lock( m_mutex );

    auto& buffer = getBuffer( txId );

    buffer.push( input );

    switch( buffer.getState() ) {
    case PinCodeState::Valid:
        dispatchPayload( { txId, std::move( buffer ).releaseBuffer(), PinCodeState::Valid } );
        m_buffers.erase( txId );
        break;

    case PinCodeState::Partial:
        dispatchPayload( { txId, buffer.getBuffer(), PinCodeState::Partial } );
        break;

    default:
        break;
    }
}

PinCodeBuffer& PinCodeController::getBuffer( uint32_t txId ) {
    auto it = m_buffers.find( txId );
    if( it != m_buffers.end() && it->second->getState() == PinCodeState::Timeout ) {
        m_buffers.erase( it );
        it = m_buffers.end();
    }

    if( it == m_buffers.end() ) {
        auto pinLength = m_devices.getPinCodeLength( txId );
        auto inputTimeout = m_devices.getPinCodeInputTimeout( txId );

        auto buffer = std::make_unique< PinCodeBuffer >( pinLength, inputTimeout, [this, txId] {
            std::unique_lock< std::mutex > lock( m_mutex );
            dispatchPayload( { txId, {}, PinCodeState::Timeout } );
        } );

        it = m_buffers.emplace_hint( it, txId, std::move( buffer ) );
    }

    return *it->second;
}

void PinCodeController::reset() {
    m_buffers.clear();
}

}  // namespace TwoGig
