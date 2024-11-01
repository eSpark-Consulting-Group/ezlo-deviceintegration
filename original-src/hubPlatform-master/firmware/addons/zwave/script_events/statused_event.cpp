#include "zwave/script_events/statused_event.h"
#include "zwave/common/zwave_errors_utils.h"

#include "znet/converters/mode_status_converter.hpp"

namespace HubPlatform {
namespace Zwave {

StatusedEvent::StatusedEvent( std::string base_command_type, znet::ModeStatus status, znet::ErrorOpt errorDetails )
: Services::ScriptEvent( getEventType( base_command_type, status ) )
, m_errorDetails( std::move( errorDetails ) ) {}

std::string StatusedEvent::getEventType( std::string base_command_type, znet::ModeStatus status ) {
    switch( status ) {
    case znet::ModeStatus::Invoked:
    case znet::ModeStatus::Started:
    case znet::ModeStatus::Finished:
        return base_command_type + "_" + znet::mode_status_converter::toString( status );

    case znet::ModeStatus::Timeout:
        return base_command_type + "_" + znet::mode_status_converter::toString( znet::ModeStatus::Finished ) + "_" +
               znet::mode_status_converter::toString( status );

    case znet::ModeStatus::Failed:
        return base_command_type + "_" + znet::mode_status_converter::toString( znet::ModeStatus::Finished ) + "_" +
               Resources::ScriptEvent::Name::ErrorSuffix;

    default:
        return "Unknown";
    }
}

Json StatusedEvent::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ScriptEvent::toJson();
    addErrorDetails( result, m_errorDetails );
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
