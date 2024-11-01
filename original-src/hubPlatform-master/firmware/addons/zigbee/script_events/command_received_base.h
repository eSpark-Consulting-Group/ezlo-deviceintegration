#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class CommandReceivedBase : public NodeEvent {
protected:
    explicit CommandReceivedBase( std::string eventName,
                                  uint16_t nodeId,
                                  uint16_t endpointId,
                                  const std::string& clusterName,
                                  uint8_t commandId,
                                  const std::string& commandName );

public:
    Json toJson() const override;

private:
    uint16_t m_endpointId;
    std::string m_clusterName;
    uint8_t m_commandId;
    std::string m_commandName;
};

}  // namespace Zigbee
}  // namespace HubPlatform
