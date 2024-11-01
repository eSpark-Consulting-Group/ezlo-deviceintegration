#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class SceneActivated : public ValueEvent {
public:
    SceneActivated( uint8_t nodeId, uint8_t channelId, uint8_t sceneNum );

    Json toJson() const final;

private:
    uint8_t m_sceneNum;
};

}  // namespace Zwave
}  // namespace HubPlatform
