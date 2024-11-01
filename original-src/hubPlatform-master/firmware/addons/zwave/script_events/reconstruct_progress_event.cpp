#include "zwave/script_events/reconstruct_progress_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

ReconstructProgressEvent::ReconstructProgressEvent( uint8_t devices, uint8_t current )
: Services::ScriptEvent( Resources::ScriptEvent::Name::ReconstructProgress )
, m_devices( devices )
, m_current( current ) {}

Json ReconstructProgressEvent::toJson() const {
    auto result = Services::ScriptEvent::toJson();
    result[ Resources::ScriptEvent::Param::ReconstructDevices ] = m_devices;
    result[ Resources::ScriptEvent::Param::CurrentReconstructedDevice ] = m_current;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
