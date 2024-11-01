#pragma once

#include "2gig/resources/action_status.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace TwoGig {

struct DeviceDiscoveringStatusUpdated : public Services::ScriptEvent {
    DeviceDiscoveringStatusUpdated( ActionStatus::Enum status );

    Json toJson() const override;

private:
    ActionStatus::Enum m_status;
};

}  // namespace TwoGig
}  // namespace HubPlatform
