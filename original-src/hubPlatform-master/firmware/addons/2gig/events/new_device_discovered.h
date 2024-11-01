#pragma once

#include "2gig/device_type.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace TwoGig {

struct NewDeviceDiscovered : public Services::ScriptEvent {
    NewDeviceDiscovered( uint32_t serial, ::TwoGig::DeviceType::Enum deviceType );

    Json toJson() const override;

private:
    uint32_t m_serial;
    ::TwoGig::DeviceType::Enum m_deviceType;
};

}  // namespace TwoGig
}  // namespace HubPlatform
