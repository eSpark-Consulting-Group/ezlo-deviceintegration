#pragma once

#include "2gig/device_type.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace TwoGig {

struct DeviceRemoved : public Services::ScriptEvent {
    DeviceRemoved( const std::vector< uint32_t >& txIds );

    Json toJson() const override;

private:
    const std::vector< uint32_t >& m_txIds;
};

}  // namespace TwoGig
}  // namespace HubPlatform
