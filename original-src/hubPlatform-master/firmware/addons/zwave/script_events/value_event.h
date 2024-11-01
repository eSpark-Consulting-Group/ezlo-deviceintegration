#pragma once

#include "zwave/script_events/node_event.h"

namespace HubPlatform {
namespace Zwave {

class ValueEvent : public NodeEvent {
public:
    ValueEvent( std::string eventName, uint8_t nodeId, uint8_t classId, uint8_t channelId );

    Json toJson() const override;

private:
    uint8_t m_classId;
    uint8_t m_channelId;
};

}  // namespace Zwave
}  // namespace HubPlatform
