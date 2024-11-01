#pragma once

#include "service_interfaces/script_event.h"
#include "zwave/common/auth_mode_utils.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class S2SelectAuthenticationModes final : public Services::ScriptEvent {
public:
    S2SelectAuthenticationModes( AuthModeUtils::Modes modes );

    Json toJson() const override;

private:
    AuthModeUtils::Modes m_modes;
};

}  // namespace Zwave
}  // namespace HubPlatform
