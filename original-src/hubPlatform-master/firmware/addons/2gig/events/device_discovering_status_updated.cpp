#include "2gig/events/device_discovering_status_updated.h"
#include "2gig/resources/resources.h"
#include "2gig/resources/enum_utils.h"

namespace HubPlatform {
namespace TwoGig {

DeviceDiscoveringStatusUpdated::DeviceDiscoveringStatusUpdated( ActionStatus::Enum status )
: Services::ScriptEvent( Resources::Events::ActionStatusUpdated )
, m_status( status ) {}

Json DeviceDiscoveringStatusUpdated::toJson() const {
    using namespace Resources;

    auto result = Services::ScriptEvent::toJson();

    result[ Events::Parameters::Action ] = "device_discovering";
    result[ Events::Parameters::Status ] = EnumUtils< ActionStatus >::toString( m_status );

    return result;
}

}  // namespace TwoGig
}  // namespace HubPlatform
