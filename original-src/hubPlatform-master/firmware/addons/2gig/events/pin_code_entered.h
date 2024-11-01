#pragma once

#include "2gig/device_type.h"
#include "2gig/pin_code_state.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace TwoGig {

struct PinCodeEntered : public Services::ScriptEvent {
    PinCodeEntered( uint32_t serial, const std::string& pin, ::TwoGig::PinCodeState::Enum pinState );

    Json toJson() const override;

private:
    uint32_t m_serial;
    std::string m_pin;
    ::TwoGig::PinCodeState::Enum m_pinState;
};

}  // namespace TwoGig
}  // namespace HubPlatform
