#include "2gig/events/pin_code_entered.h"
#include "2gig/resources/resources.h"
#include "2gig/resources/enum_utils.h"

namespace HubPlatform {
namespace TwoGig {

PinCodeEntered::PinCodeEntered( uint32_t serial, const std::string& pin, ::TwoGig::PinCodeState::Enum pinState )
: Services::ScriptEvent( Resources::Events::PinCodeEntered )
, m_serial( serial )
, m_pin( pin )
, m_pinState( pinState ) {}

Json PinCodeEntered::toJson() const {
    using namespace Resources;

    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::Pin ] = m_pin;
    result[ Events::Parameters::PinState ] = EnumUtils< ::TwoGig::PinCodeState >::toString( m_pinState );

    return result;
}

}  // namespace TwoGig
}  // namespace HubPlatform
