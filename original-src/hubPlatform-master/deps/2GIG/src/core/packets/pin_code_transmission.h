#pragma once

#include "2gig/pin_code_state.h"

#include <string>

namespace TwoGig {

struct PinCodeTransmission {
    PinCodeTransmission( uint32_t txId, std::string pin, PinCodeState::Enum pinState )
    : txId( txId )
    , pin( std::move( pin ) )
    , pinState( pinState ) {}

public:
    uint32_t txId;
    std::string pin;
    PinCodeState::Enum pinState;
};

}  // namespace TwoGig
