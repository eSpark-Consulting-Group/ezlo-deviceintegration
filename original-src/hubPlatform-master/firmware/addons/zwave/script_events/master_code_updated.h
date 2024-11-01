#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class MasterCodeUpdated : public ValueEvent {
public:
    MasterCodeUpdated( uint8_t nodeId, uint8_t channelId, std::string masterCode );

    Json toJson() const override;

private:
    std::string m_masterCode;
};

}  // namespace Zwave
}  // namespace HubPlatform
