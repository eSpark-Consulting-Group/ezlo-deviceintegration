#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class UserCodesChecksumUpdated : public ValueEvent {
public:
    UserCodesChecksumUpdated( uint8_t nodeId, uint8_t channelId, uint16_t checksum );

    Json toJson() const override;

private:
    uint16_t m_checksum;
};

}  // namespace Zwave
}  // namespace HubPlatform
