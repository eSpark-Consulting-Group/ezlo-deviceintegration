#pragma once

#include "2gig/device_descriptor.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace TwoGig {

struct DeviceAdded : public Services::ScriptEvent {
    DeviceAdded( const ::TwoGig::DeviceDescriptor& descriptor );

    Json toJson() const override;

private:
    const ::TwoGig::DeviceDescriptor m_descriptor;
};

}  // namespace TwoGig
}  // namespace HubPlatform
