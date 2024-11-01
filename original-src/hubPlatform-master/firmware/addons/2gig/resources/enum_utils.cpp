#include "2gig/resources/enum_utils.h"
#include "2gig/resources/action_status.h"

#include "2gig/device_type.h"
#include "2gig/pad_button.h"
#include "2gig/pin_code_state.h"

namespace HubPlatform {
namespace TwoGig {
namespace Resources {

template <>
EnumUtils< ::TwoGig::DeviceType > EnumUtils< ::TwoGig::DeviceType >::ms_impl( {
    { "smoke_detector", ::TwoGig::DeviceType::SmokeDetector },
    { "panic_button", ::TwoGig::DeviceType::PanicButton },
    { "pir_detector", ::TwoGig::DeviceType::PirDetector },
    { "flood_temperature", ::TwoGig::DeviceType::FloodTemperature },
    { "honeywell_sensor", ::TwoGig::DeviceType::HoneywellSensor },
    { "glass_break_detector", ::TwoGig::DeviceType::GlassBreakDetector },
    { "door_window_sensor", ::TwoGig::DeviceType::DoorWindowSensor },
    { "co_detector", ::TwoGig::DeviceType::CoDetector },
    { "tilt_detector", ::TwoGig::DeviceType::TiltDetector },
    { "remote_keyfob", ::TwoGig::DeviceType::RemoteKeyFob },
    { "smoke_co_detector", ::TwoGig::DeviceType::SmokeCoDetector },
    { "door_window_with_wire_sensor", ::TwoGig::DeviceType::DoorWindowWithWireSensor },
    { "repeater", ::TwoGig::DeviceType::Repeater },
    { "keypad", ::TwoGig::DeviceType::Keypad },
} );

template <>
EnumUtils< ::TwoGig::PadButton > EnumUtils< ::TwoGig::PadButton >::ms_impl( {
    { "1", ::TwoGig::PadButton::Num_1 },
    { "2", ::TwoGig::PadButton::Num_2 },
    { "3", ::TwoGig::PadButton::Num_3 },
    { "4", ::TwoGig::PadButton::Num_4 },
    { "5", ::TwoGig::PadButton::Num_5 },
    { "6", ::TwoGig::PadButton::Num_6 },
    { "7", ::TwoGig::PadButton::Num_7 },
    { "8", ::TwoGig::PadButton::Num_8 },
    { "9", ::TwoGig::PadButton::Num_9 },
    { "*", ::TwoGig::PadButton::Asterisk },
    { "0", ::TwoGig::PadButton::Num_0 },
    { "#", ::TwoGig::PadButton::Pound },
    { "stay", ::TwoGig::PadButton::Stay },
    { "away", ::TwoGig::PadButton::Away },
    { "police", ::TwoGig::PadButton::Police },
    { "fire", ::TwoGig::PadButton::Fire },
} );

template <>
EnumUtils< ::TwoGig::PinCodeState > EnumUtils< ::TwoGig::PinCodeState >::ms_impl( {
    { "partial", ::TwoGig::PinCodeState::Partial },
    { "valid", ::TwoGig::PinCodeState::Valid },
    { "timeout", ::TwoGig::PinCodeState::Timeout },
} );

template <>
EnumUtils< ActionStatus > EnumUtils< ActionStatus >::ms_impl( {
    { "started", ActionStatus::Started },
    { "finished", ActionStatus::Finished },
    { "failed", ActionStatus::Failed },
    { "timeout", ActionStatus::Timeout },
} );

template < typename EnumT >
EnumUtils< EnumT >::EnumUtils( std::map< std::string, Enum >&& lookup )
: m_valueByString( std::move( lookup ) ) {
    for( auto& stringValue : m_valueByString )
        m_stringByValue.insert( { stringValue.second, &stringValue.first } );
}

template < typename EnumT >
typename EnumUtils< EnumT >::Enum EnumUtils< EnumT >::fromString( const std::string& str ) {
    auto it = ms_impl.m_valueByString.find( str );
    if( it != ms_impl.m_valueByString.end() )
        return it->second;

    throw std::invalid_argument( "Unknown value: '" + str + "'" );
}

template < typename EnumT >
const std::string& EnumUtils< EnumT >::toString( Enum value ) {
    auto it = ms_impl.m_stringByValue.find( value );
    if( it != ms_impl.m_stringByValue.end() )
        return *it->second;

    throw std::invalid_argument( "Unknown value: '" + std::to_string( value ) + "'" );
}

template struct EnumUtils< ::TwoGig::DeviceType >;
template struct EnumUtils< ::TwoGig::PadButton >;
template struct EnumUtils< ::TwoGig::PinCodeState >;
template struct EnumUtils< ActionStatus >;

}  // namespace Resources
}  // namespace TwoGig
}  // namespace HubPlatform
