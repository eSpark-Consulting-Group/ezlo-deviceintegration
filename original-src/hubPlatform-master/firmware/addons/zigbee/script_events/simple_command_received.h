#pragma once

#include "zigbee/script_events/command_received_base.h"

namespace HubPlatform {
namespace Zigbee {

enum class CommandType;

class SimpleCommandReceived : public CommandReceivedBase {
public:
    explicit SimpleCommandReceived( uint16_t nodeId,
                                    uint16_t endpointId,
                                    const std::string& clusterName,
                                    uint8_t commandId,
                                    const std::string& commandName );
};

}  // namespace Zigbee
}  // namespace HubPlatform
