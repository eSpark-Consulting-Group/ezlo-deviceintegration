#pragma once

#include "2gig/client.h"

#include "2gig/events/device_status_updated.h"

namespace HubPlatform {
namespace TwoGig {

class Addon;

class ClientImpl : public ::TwoGig::Client {
public:
    ClientImpl( Addon& addon );

    void onReset() override;

    void onNewDeviceDiscovered( uint32_t serial, ::TwoGig::DeviceType::Enum deviceType ) override;

    void onDeviceAdded( const ::TwoGig::DeviceDescriptor& descriptor ) override;
    void onDeviceRemoved( const std::vector< uint32_t >& txIds ) override;

    void onDeviceReachableChanged( uint32_t serial, bool reachable ) override;

    void onSmokeDetectorUpdated( uint32_t serial, ::TwoGig::SmokeDetectorStatus status ) override;
    void onPanicButtonUpdated( uint32_t serial, ::TwoGig::PanicButtonStatus status ) override;
    void onPirDetectorUpdated( uint32_t serial, ::TwoGig::PirDetectorStatus status ) override;
    void onFloodTemperatureUpdated( uint32_t serial, ::TwoGig::FloodTemperatureStatus status ) override;
    void onGlassBreakDetectorUpdated( uint32_t serial, ::TwoGig::GlassBreakStatus status ) override;
    void onDoorWindowSensorUpdated( uint32_t serial, ::TwoGig::DoorWindowSensorStatus status ) override;
    void onCoDetectorUpdated( uint32_t serial, ::TwoGig::CoDetectorStatus status ) override;
    void onTiltDetectorUpdated( uint32_t serial, ::TwoGig::TiltDetectorStatus status ) override;
    void onRemoteKeyFobUpdated( uint32_t serial, ::TwoGig::RemoteKeyFobStatus status ) override;
    void onPadUpdated( uint32_t serial, ::TwoGig::PadStatus status ) override;
    void onPinUpdated( uint32_t serial, const std::string& pin, ::TwoGig::PinCodeState::Enum pinState ) override;
    void onRepeaterUpdated( uint32_t serial, ::TwoGig::RepeaterStatus status ) override;
    void onHoneywellSensorUpdated( uint32_t serial, ::TwoGig::HoneywellSensorStatus status ) override;

private:
    template < typename StatusT >
    static DeviceStatusUpdated< StatusT > makeDeviceStatusUpdatedEvent( uint32_t serial, StatusT status );

private:
    Addon& m_addon;
};

}  // namespace TwoGig
}  // namespace HubPlatform
