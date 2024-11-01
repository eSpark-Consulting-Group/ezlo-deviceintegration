#pragma once

#include "2gig/device_status.h"
#include "2gig/device_type.h"
#include "2gig/pin_code_state.h"

#include <string>
#include <vector>

namespace TwoGig {

struct DeviceDescriptor;

struct Client {
    virtual ~Client() = default;

    virtual void onReset() = 0;

    virtual void onNewDeviceDiscovered( uint32_t serial, DeviceType::Enum deviceType ) = 0;

    virtual void onDeviceAdded( const DeviceDescriptor& descriptor ) = 0;
    virtual void onDeviceRemoved( const std::vector< uint32_t >& txIds ) = 0;

    virtual void onDeviceReachableChanged( uint32_t serial, bool reachable ) = 0;

    virtual void onSmokeDetectorUpdated( uint32_t serial, SmokeDetectorStatus status ) = 0;
    virtual void onPanicButtonUpdated( uint32_t serial, PanicButtonStatus status ) = 0;
    virtual void onPirDetectorUpdated( uint32_t serial, PirDetectorStatus status ) = 0;
    virtual void onFloodTemperatureUpdated( uint32_t serial, FloodTemperatureStatus status ) = 0;
    virtual void onGlassBreakDetectorUpdated( uint32_t serial, GlassBreakStatus status ) = 0;
    virtual void onDoorWindowSensorUpdated( uint32_t serial, DoorWindowSensorStatus status ) = 0;
    virtual void onCoDetectorUpdated( uint32_t serial, CoDetectorStatus status ) = 0;
    virtual void onTiltDetectorUpdated( uint32_t serial, TiltDetectorStatus status ) = 0;
    virtual void onRemoteKeyFobUpdated( uint32_t serial, RemoteKeyFobStatus status ) = 0;
    virtual void onPadUpdated( uint32_t serial, PadStatus status ) = 0;
    virtual void onPinUpdated( uint32_t serial, const std::string& pin, PinCodeState::Enum pinState ) = 0;
    virtual void onRepeaterUpdated( uint32_t serial, RepeaterStatus status ) = 0;
    virtual void onHoneywellSensorUpdated( uint32_t serial, HoneywellSensorStatus status ) = 0;
};

}  // namespace TwoGig
