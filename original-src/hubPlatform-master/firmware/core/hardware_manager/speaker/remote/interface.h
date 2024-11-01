#pragma once
//
// Abstract remote interface for HW/Speaker
//
#include "json_utils/json_fwd.h"
#include "hardware_manager/speaker/model/playing_info.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class SpeakerRemote {
public:
    using SoundId = std::string;

public:
    virtual ~SpeakerRemote() = default;

public:  // interface
    virtual SoundId play( std::string songFilePath,
                          std::int64_t time,
                          std::uint8_t volume,
                          std::string plugin,
                          std::string handlerScript,
                          Json userData ) = 0;
    virtual bool stop( const SoundId& ) = 0;
    virtual PlayingInfo getPlayingInfo( const SoundId& ) = 0;
};

using SpeakerRemoteUPtr = std::unique_ptr< SpeakerRemote >;

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform