#include "zwave/script_events/s2_select_authentication_modes.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

S2SelectAuthenticationModes::S2SelectAuthenticationModes( AuthModeUtils::Modes modes )
: Services::ScriptEvent( Resources::ScriptEvent::Name::S2SelectAuthenticationModes )
, m_modes( std::move( modes ) ) {}

Json S2SelectAuthenticationModes::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::Modes ] = m_modes;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
