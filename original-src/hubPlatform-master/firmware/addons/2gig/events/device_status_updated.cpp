#include "2gig/events/device_status_updated.h"
#include "2gig/resources/enum_utils.h"
#include "2gig/resources/resources.h"

#include "2gig/device_status.h"
#include "2gig/pad_button.h"

namespace HubPlatform {
namespace TwoGig {

using namespace Resources;

template < typename >
struct StatusEventName {
    static const char* Value;
};

template <>
const char* StatusEventName< ::TwoGig::DoorWindowSensorStatus >::Value = Events::DoorWindowSensorStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::SmokeDetectorStatus >::Value = Events::SmokeDetectorStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::CoDetectorStatus >::Value = Events::CoDetectorStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::PirDetectorStatus >::Value = Events::PirDetectorStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::GlassBreakStatus >::Value = Events::GlassBreakStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::FloodTemperatureStatus >::Value = Events::FloodTemperatureStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::TiltDetectorStatus >::Value = Events::TiltDetectorStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::RemoteKeyFobStatus >::Value = Events::RemoteKeyFobStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::PanicButtonStatus >::Value = Events::PanicButtonStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::PadStatus >::Value = Events::PadStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::RepeaterStatus >::Value = Events::RepeaterStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::HoneywellSensorStatus >::Value = Events::HoneywellSensorStatusUpdated;

template <>
const char* StatusEventName< ::TwoGig::ReachableStatus >::Value = Events::ReachableStatusUpdated;

template < typename StatusT >
DeviceStatusUpdated< StatusT >::DeviceStatusUpdated( uint32_t serial, StatusT status )
: Services::ScriptEvent( StatusEventName< StatusT >::Value )
, m_serial( serial )
, m_status( status ) {}

template <>
Json DeviceStatusUpdated< ::TwoGig::DoorWindowSensorStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::ReedClosed ] = m_status.reedClosed;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::ExternalContact ] = m_status.externalContact;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::SmokeDetectorStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::FreezeAlarm ] = m_status.freezeAlarm;
    result[ Events::Parameters::HeatAlarm ] = m_status.heatAlarm;
    result[ Events::Parameters::SmokeAlarm ] = m_status.smokeAlarm;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::CoDetectorStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::CoAlarm ] = m_status.coAlarm;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::PirDetectorStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::MotionDetected ] = m_status.motionDetected;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::GlassBreakStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::GlassBreakDetected ] = m_status.glassBreakDetected;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::FloodTemperatureStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::FreezeAlarm ] = m_status.freezeAlarm;
    result[ Events::Parameters::HeatAlarm ] = m_status.heatAlarm;
    result[ Events::Parameters::WaterDetected ] = m_status.waterDetected;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::TiltDetectorStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::TiltDetected ] = m_status.tiltDetected;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::RemoteKeyFobStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::Button1Pressed ] = m_status.button1Pressed;
    result[ Events::Parameters::Button2Pressed ] = m_status.button2Pressed;
    result[ Events::Parameters::Button3Pressed ] = m_status.button3Pressed;
    result[ Events::Parameters::Button4Pressed ] = m_status.button4Pressed;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::PanicButtonStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::ButtonPressed ] = m_status.buttonPressed;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::PadStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    if( !m_status.supervisory ) {
        result[ Events::Parameters::ButtonPressed ] =
            EnumUtils< ::TwoGig::PadButton >::toString( m_status.buttonPressed );
    }
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::RepeaterStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::JamDetected ] = m_status.jamDetected;
    result[ Events::Parameters::AcFailed ] = m_status.acFailed;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::HoneywellSensorStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::Loop1 ] = m_status.loop1;
    result[ Events::Parameters::Loop2 ] = m_status.loop2;
    result[ Events::Parameters::Loop3 ] = m_status.loop3;
    result[ Events::Parameters::TamperDetected ] = m_status.tamperDetected;
    result[ Events::Parameters::LowBattery ] = m_status.lowBattery;

    return result;
}

template <>
Json DeviceStatusUpdated< ::TwoGig::ReachableStatus >::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::Reachable ] = m_status.isReachable;

    return result;
}

template struct DeviceStatusUpdated< ::TwoGig::DoorWindowSensorStatus >;
template struct DeviceStatusUpdated< ::TwoGig::SmokeDetectorStatus >;
template struct DeviceStatusUpdated< ::TwoGig::CoDetectorStatus >;
template struct DeviceStatusUpdated< ::TwoGig::PirDetectorStatus >;
template struct DeviceStatusUpdated< ::TwoGig::GlassBreakStatus >;
template struct DeviceStatusUpdated< ::TwoGig::FloodTemperatureStatus >;
template struct DeviceStatusUpdated< ::TwoGig::TiltDetectorStatus >;
template struct DeviceStatusUpdated< ::TwoGig::RemoteKeyFobStatus >;
template struct DeviceStatusUpdated< ::TwoGig::PanicButtonStatus >;
template struct DeviceStatusUpdated< ::TwoGig::PadStatus >;
template struct DeviceStatusUpdated< ::TwoGig::RepeaterStatus >;
template struct DeviceStatusUpdated< ::TwoGig::HoneywellSensorStatus >;
template struct DeviceStatusUpdated< ::TwoGig::ReachableStatus >;

}  // namespace TwoGig
}  // namespace HubPlatform
