#include "core/pipeline/helpers/pin_code_buffer.h"

namespace TwoGig {

PinCodeBuffer::PinCodeBuffer( uint8_t length, std::chrono::seconds inputTimeout, std::function< void() > onTimeout )
: m_length( length )
, m_inputTimeout( inputTimeout )
, m_timeoutTask( [this, onTimeout] { ( m_state = PinCodeState::Timeout ), onTimeout(); } )
, m_state( PinCodeState::Partial ) {}

void PinCodeBuffer::push( char input ) {
    m_timeoutTask.stop();

    m_buffer += input;

    if( m_buffer.length() == m_length )
        m_state = PinCodeState::Valid;
    else
        m_timeoutTask.reschedule( m_inputTimeout );
}

PinCodeState::Enum PinCodeBuffer::getState() const {
    return m_state;
}

const std::string& PinCodeBuffer::getBuffer() const& {
    return m_buffer;
}

std::string PinCodeBuffer::releaseBuffer() && {
    return std::move( m_buffer );
}

}  // namespace TwoGig
