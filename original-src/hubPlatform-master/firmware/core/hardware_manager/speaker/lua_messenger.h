#pragma once

#include "hardware_manager/speaker/imessanger.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class IEnvironment;

class LuaMessenger : public IMessenger {
public:
    explicit LuaMessenger( IEnvironment& environment );

public:
    bool sendEvent( const PlayingInfo& info, PlayingStatus status, ErrorInfo::Code errorCode ) override;

private:
    IEnvironment& m_environment;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
