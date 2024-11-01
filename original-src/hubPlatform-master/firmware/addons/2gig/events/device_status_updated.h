#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace TwoGig {

template < typename StatusT >
struct DeviceStatusUpdated : public Services::ScriptEvent {
    DeviceStatusUpdated( uint32_t serial, StatusT status );

    Json toJson() const override;

private:
    uint32_t m_serial;
    StatusT m_status;
};

}  // namespace TwoGig
}  // namespace HubPlatform
