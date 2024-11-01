#pragma once

#include "zwave/resources/zwave_resources.h"

#include "service_interfaces/script_event.h"

#include <znet/znet_command.hpp>
#include <znet/znet_error_message.hpp>

namespace HubPlatform {
namespace Zwave {

class ModuleReset : public Services::ScriptEvent {
public:
    ModuleReset( znet::ModeStatus status, znet::ErrorOpt errorDetails );

    Json toJson() const override;

private:
    const znet::ModeStatus m_status;
    const znet::ErrorOpt m_errorDetails;
};

}  // namespace Zwave
}  // namespace HubPlatform
