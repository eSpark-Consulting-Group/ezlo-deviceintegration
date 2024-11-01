#pragma once

#include "2gig/client.h"

namespace TwoGig {
namespace Test {

class FakeClient : public Client {
public:
    void onReset() override {}

    void onNewDeviceDiscovered( uint32_t serial, DeviceType::Enum deviceType ) override {}

    void onDeviceAdded( const DeviceDescriptor& descriptor ) override {}
    void onDeviceRemoved( const std::vector< uint32_t >& txIds ) override {}

    void onDeviceReachableChanged( uint32_t serial, bool reachable ) override {}

    void onSmokeDetectorUpdated( uint32_t serial, ::TwoGig::SmokeDetectorStatus status ) override {}
    void onPanicButtonUpdated( uint32_t serial, ::TwoGig::PanicButtonStatus status ) override {}
    void onPirDetectorUpdated( uint32_t serial, ::TwoGig::PirDetectorStatus status ) override {}
    void onFloodTemperatureUpdated( uint32_t serial, ::TwoGig::FloodTemperatureStatus status ) override {}
    void onGlassBreakDetectorUpdated( uint32_t serial, ::TwoGig::GlassBreakStatus status ) override {}
    void onDoorWindowSensorUpdated( uint32_t serial, ::TwoGig::DoorWindowSensorStatus status ) override {}
    void onCoDetectorUpdated( uint32_t serial, ::TwoGig::CoDetectorStatus status ) override {}
    void onTiltDetectorUpdated( uint32_t serial, ::TwoGig::TiltDetectorStatus status ) override {}
    void onRemoteKeyFobUpdated( uint32_t serial, ::TwoGig::RemoteKeyFobStatus status ) override {}
    void onPadUpdated( uint32_t serial, ::TwoGig::PadStatus status ) override {}
    void onPinUpdated( uint32_t serial, const std::string& pin, PinCodeState::Enum pinState ) override {}
    void onRepeaterUpdated( uint32_t serial, RepeaterStatus status ) override {}
    void onHoneywellSensorUpdated( uint32_t serial, HoneywellSensorStatus status ) override {}
};

}  // namespace Test
}  // namespace TwoGig
