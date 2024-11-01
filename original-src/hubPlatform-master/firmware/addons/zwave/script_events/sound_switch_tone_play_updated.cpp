#include "zwave/script_events/sound_switch_tone_play_updated.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

SoundSwitchTonePlayUpdated::SoundSwitchTonePlayUpdated( uint8_t nodeId,
                                                        uint8_t channelId,
                                                        uint8_t toneId,
                                                        uint8_t volume )
: ValueEvent( Resources::ScriptEvent::Name::SoundSwitchTonePlayUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SOUND_SWITCH,
              channelId )
, m_toneId( toneId )
, m_volume( volume ) {}

Json SoundSwitchTonePlayUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent;

    result[ Param::Sound::ToneId ] = m_toneId;
    result[ Param::Sound::Volume ] = m_volume;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
