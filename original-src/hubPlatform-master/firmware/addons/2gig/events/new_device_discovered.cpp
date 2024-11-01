#include "2gig/events/new_device_discovered.h"
#include "2gig/resources/resources.h"
#include "2gig/resources/enum_utils.h"

namespace HubPlatform {
namespace TwoGig {

NewDeviceDiscovered::NewDeviceDiscovered( uint32_t serial, ::TwoGig::DeviceType::Enum deviceType )
: Services::ScriptEvent( Resources::Events::NewDeviceDiscovered )
, m_serial( serial )
, m_deviceType( deviceType ) {}

Json NewDeviceDiscovered::toJson() const {
    using namespace Resources;

    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Serial ] = m_serial;
    result[ Events::Parameters::DeviceType ] = EnumUtils< ::TwoGig::DeviceType >::toString( m_deviceType );

    return result;
}

}  // namespace TwoGig
}  // namespace HubPlatform
