#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class SimpleAvControlSetReceived : public ValueEvent {
public:
    SimpleAvControlSetReceived( uint8_t nodeId,
                                uint8_t channelId,
                                uint8_t sequenceNum,
                                const std::string& keyAttribute,
                                std::vector< uint16_t >&& commands );

    Json toJson() const final;

private:
    uint8_t m_sequenceNum;
    std::string m_keyAttribute;
    std::vector< uint16_t > m_commands;
};

}  // namespace Zwave
}  // namespace HubPlatform
