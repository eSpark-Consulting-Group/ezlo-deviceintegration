#pragma once
//
// Abstract remote interface for HW/Speaker
//
#include "hardware_manager/speaker/remote/interface.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class SpeakerRemoteImpl : public SpeakerRemote {
public:
    explicit SpeakerRemoteImpl( Services::Service& hostService );
    ~SpeakerRemoteImpl() override = default;

public:  // interface
    SoundId play( std::string songFilePath,
                  std::int64_t time,
                  std::uint8_t volume,
                  std::string plugin,
                  std::string handlerScript,
                  Json userData ) override;
    bool stop( const SoundId& ) override;
    PlayingInfo getPlayingInfo( const SoundId& ) override;

private:
    Services::Service& m_hostService;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform