#pragma once

#include "hardware_manager/hub_state/model/hub_state.h"
#include "hardware_manager/effect/effect.h"

namespace HubPlatform {
namespace HardwareManager {

class HubStateIndicationManager {
public:
    // Foreground state indication
    virtual void setEventEffect( const std::string& effectName ) = 0;

    // Background state indication
    virtual void setHubStateEffect( const std::string& effectName ) = 0;
    virtual void setHubStateIndicator( const std::string& indicator, bool state ) = 0;
    virtual void setHubStateIndicatorBlinking( const std::string& indicator, int msOff, int msOn ) = 0;
    virtual void indicateDown() = 0;
    virtual void setHubStateCustomEffectSettings( const Color color, const BlinkingType blinking ) = 0;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
