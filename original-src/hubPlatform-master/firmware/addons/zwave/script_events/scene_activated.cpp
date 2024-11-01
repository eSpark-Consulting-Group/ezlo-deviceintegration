#include "zwave/script_events/scene_activated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

SceneActivated::SceneActivated( uint8_t nodeId, uint8_t channelId, uint8_t sceneNum )
: ValueEvent( Resources::ScriptEvent::Name::SceneActivated, nodeId, ZNET_COMMAND_CLASS_SCENE_ACTIVATION, channelId )
, m_sceneNum( sceneNum ) {}

Json SceneActivated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::SceneId ] = m_sceneNum;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
