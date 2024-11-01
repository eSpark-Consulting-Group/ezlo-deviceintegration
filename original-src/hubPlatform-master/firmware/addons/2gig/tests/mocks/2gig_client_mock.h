#pragma once

#include "2gig/client.h"
#include "2gig/device_descriptor.h"

#include <gmock/gmock.h>

namespace HubPlatform {
namespace TwoGig {

class ClientMock : public ::TwoGig::Client {
public:
    ClientMock() {}

    MOCK_METHOD0( onReset, void() );

    MOCK_METHOD2( onNewDeviceDiscovered, void( uint32_t, ::TwoGig::DeviceType::Enum ) );

    MOCK_METHOD1( onDeviceAdded, void( const ::TwoGig::DeviceDescriptor& ) );
    MOCK_METHOD1( onDeviceRemoved, void( const std::vector< uint32_t >& ) );

    MOCK_METHOD2( onDeviceReachableChanged, void( uint32_t, bool ) );

    MOCK_METHOD2( onSmokeDetectorUpdated, void( uint32_t, ::TwoGig::SmokeDetectorStatus ) );
    MOCK_METHOD2( onPanicButtonUpdated, void( uint32_t, ::TwoGig::PanicButtonStatus ) );
    MOCK_METHOD2( onPirDetectorUpdated, void( uint32_t, ::TwoGig::PirDetectorStatus ) );
    MOCK_METHOD2( onFloodTemperatureUpdated, void( uint32_t, ::TwoGig::FloodTemperatureStatus ) );
    MOCK_METHOD2( onGlassBreakDetectorUpdated, void( uint32_t, ::TwoGig::GlassBreakStatus ) );
    MOCK_METHOD2( onDoorWindowSensorUpdated, void( uint32_t, ::TwoGig::DoorWindowSensorStatus ) );
    MOCK_METHOD2( onCoDetectorUpdated, void( uint32_t, ::TwoGig::CoDetectorStatus ) );
    MOCK_METHOD2( onTiltDetectorUpdated, void( uint32_t, ::TwoGig::TiltDetectorStatus ) );
    MOCK_METHOD2( onRemoteKeyFobUpdated, void( uint32_t, ::TwoGig::RemoteKeyFobStatus ) );
    MOCK_METHOD2( onPadUpdated, void( uint32_t, ::TwoGig::PadStatus ) );
    MOCK_METHOD3( onPinUpdated, void( uint32_t, const std::string&, ::TwoGig::PinCodeState::Enum ) );
    MOCK_METHOD2( onRepeaterUpdated, void( uint32_t, ::TwoGig::RepeaterStatus ) );
    MOCK_METHOD2( onHoneywellSensorUpdated, void( uint32_t, ::TwoGig::HoneywellSensorStatus ) );
};

}  // namespace TwoGig
}  // namespace HubPlatform
