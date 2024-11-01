#pragma once

#include "hardware_manager/speaker/model/playing_status.h"
#include "hardware_manager/speaker/model/speaker_errors.h"

#include <string>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class PlayingInfo;

class IMessenger {
public:
    virtual ~IMessenger() = default;

public:
    virtual bool sendEvent( const PlayingInfo& playingInfo,
                            PlayingStatus status,
                            ErrorInfo::Code errorCode = ErrorInfo::Code::NoError ) = 0;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform