#pragma once

#include "service_interfaces/script_event.h"

#include "zwave/resources/zwave_resources.h"

#include "znet/znet_command.hpp"
#include "znet/znet_error_message.hpp"

namespace HubPlatform {
namespace Zwave {

class StatusedEvent : public Services::ScriptEvent {
public:
    StatusedEvent( std::string base_command_type, znet::ModeStatus status, znet::ErrorOpt errorDetails );

    Json toJson() const;

private:
    static std::string getEventType( std::string base_command_type, znet::ModeStatus status );

private:
    znet::ErrorOpt m_errorDetails;
};

}  // namespace Zwave
}  // namespace HubPlatform
