#include "2gig/2gig_client.h"
#include "2gig/2gig_addon.h"
#include "2gig/resources/resources.h"
#include "2gig/events/new_device_discovered.h"
#include "2gig/events/device_added.h"
#include "2gig/events/device_removed.h"
#include "2gig/events/pin_code_entered.h"

namespace HubPlatform {
namespace TwoGig {

ClientImpl::ClientImpl( Addon& addon )
: m_addon( addon ) {}

void ClientImpl::onReset() {
    m_addon.emit( Services::ScriptEvent( Resources::Events::AddonReset ) );
}

void ClientImpl::onNewDeviceDiscovered( uint32_t serial, ::TwoGig::DeviceType::Enum deviceType ) {
    m_addon.emit( NewDeviceDiscovered( serial, deviceType ) );
}

void ClientImpl::onDeviceAdded( const ::TwoGig::DeviceDescriptor& descriptor ) {
    m_addon.emit( DeviceAdded( descriptor ) );
}

void ClientImpl::onDeviceRemoved( const std::vector< uint32_t >& txIds ) {
    m_addon.emit( DeviceRemoved( txIds ) );
}

void ClientImpl::onDeviceReachableChanged( uint32_t serial, bool reachable ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, ::TwoGig::ReachableStatus{ reachable } ) );
}

void ClientImpl::onSmokeDetectorUpdated( uint32_t serial, ::TwoGig::SmokeDetectorStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onPanicButtonUpdated( uint32_t serial, ::TwoGig::PanicButtonStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onPirDetectorUpdated( uint32_t serial, ::TwoGig::PirDetectorStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onFloodTemperatureUpdated( uint32_t serial, ::TwoGig::FloodTemperatureStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onGlassBreakDetectorUpdated( uint32_t serial, ::TwoGig::GlassBreakStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onDoorWindowSensorUpdated( uint32_t serial, ::TwoGig::DoorWindowSensorStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onCoDetectorUpdated( uint32_t serial, ::TwoGig::CoDetectorStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onTiltDetectorUpdated( uint32_t serial, ::TwoGig::TiltDetectorStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onRemoteKeyFobUpdated( uint32_t serial, ::TwoGig::RemoteKeyFobStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onPadUpdated( uint32_t serial, ::TwoGig::PadStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onPinUpdated( uint32_t serial, const std::string& pin, ::TwoGig::PinCodeState::Enum pinState ) {
    m_addon.emit( PinCodeEntered( serial, pin, pinState ) );
}

void ClientImpl::onRepeaterUpdated( uint32_t serial, ::TwoGig::RepeaterStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

void ClientImpl::onHoneywellSensorUpdated( uint32_t serial, ::TwoGig::HoneywellSensorStatus status ) {
    m_addon.emit( makeDeviceStatusUpdatedEvent( serial, status ) );
}

template < typename StatusT >
DeviceStatusUpdated< StatusT > ClientImpl::makeDeviceStatusUpdatedEvent( uint32_t serial, StatusT status ) {
    return DeviceStatusUpdated< StatusT >( serial, status );
}

}  // namespace TwoGig
}  // namespace HubPlatform
