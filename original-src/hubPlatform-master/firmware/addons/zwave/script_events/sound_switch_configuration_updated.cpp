#include "zwave/script_events/sound_switch_configuration_updated.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

SoundSwitchConfigurationUpdated::SoundSwitchConfigurationUpdated( uint8_t nodeId,
                                                                  uint8_t channelId,
                                                                  uint8_t defaultToneId,
                                                                  uint8_t volume )
: ValueEvent( Resources::ScriptEvent::Name::SoundSwitchConfigurationUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SOUND_SWITCH,
              channelId )
, m_defaultToneId( defaultToneId )
, m_volume( volume ) {}

Json SoundSwitchConfigurationUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent;

    result[ Param::Sound::DefaultToneId ] = m_defaultToneId;
    result[ Param::Sound::Volume ] = m_volume;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
