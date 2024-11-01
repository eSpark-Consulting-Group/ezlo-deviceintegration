#include "zwave/script_events/module_reset.h"
#include "zwave/common/zwave_errors_utils.h"

#include "znet/converters/mode_status_converter.hpp"

namespace HubPlatform {
namespace Zwave {

ModuleReset::ModuleReset( znet::ModeStatus status, znet::ErrorOpt errorDetails )
: Services::ScriptEvent( Resources::ScriptEvent::Name::ModuleReset )
, m_status( status )
, m_errorDetails( std::move( errorDetails ) ) {}

Json ModuleReset::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::ResetStatus ] = znet::mode_status_converter::toString( m_status );
    addErrorDetails( result, m_errorDetails );
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
