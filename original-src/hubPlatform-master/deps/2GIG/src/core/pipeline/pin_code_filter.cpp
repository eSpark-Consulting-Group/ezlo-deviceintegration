#include "core/pipeline/pin_code_filter.h"
#include "core/packets/packets.h"

namespace TwoGig {

PinCodeFilter::PinCodeFilter( const Logger& logger )
: PipeProducer< Transmission, 2 >( logger ) {}

void PinCodeFilter::processPayload( Transmission packet ) {
    bool isPinCode = isPinCodeInput( packet );
    dispatchPayload( packet, isPinCode ? PipeChannel::PinCodeInput : PipeChannel::StatusUpdate );
}

bool PinCodeFilter::isPinCodeInput( Transmission packet ) {
    if( packet.getDeviceType() != DeviceType::Keypad )
        return false;

    const auto status = packet.getStatus< PadStatus >();
    if( status.supervisory )
        return false;

    return status.buttonPressed >= PadButton::Num_1 && status.buttonPressed <= PadButton::Pound;
}

}  // namespace TwoGig
