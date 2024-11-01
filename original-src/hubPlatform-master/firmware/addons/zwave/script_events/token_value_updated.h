#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class TokenValueUpdated : public ValueEvent {
public:
    TokenValueUpdated( uint8_t nodeId, uint8_t classId, uint8_t channelId, const std::string& value );

    Json toJson() const final;

private:
    std::string m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
