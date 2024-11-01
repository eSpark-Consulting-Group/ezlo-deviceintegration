#pragma once

#include "2gig/pin_code_state.h"

#include "utils/thread/postponed_task.h"

#include <string>

namespace TwoGig {

struct PinCodeBuffer {
    PinCodeBuffer( uint8_t length, std::chrono::seconds inputTimeout, std::function< void() > onTimeout );

    void push( char input );
    PinCodeState::Enum getState() const;

    const std::string& getBuffer() const&;
    std::string releaseBuffer() &&;

private:
    uint8_t m_length;
    std::chrono::seconds m_inputTimeout;
    std::string m_buffer;
    PinCodeState::Enum m_state;

    PostponedTask m_timeoutTask;
};

}  // namespace TwoGig
