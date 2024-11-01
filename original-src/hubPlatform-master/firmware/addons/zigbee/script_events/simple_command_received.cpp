#include "zigbee/script_events/simple_command_received.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

SimpleCommandReceived::SimpleCommandReceived( uint16_t nodeId,
                                              uint16_t endpointId,
                                              const std::string& clusterName,
                                              uint8_t commandId,
                                              const std::string& commandName )
: CommandReceivedBase( Resources::ScriptEvent::Name::SimpleCommandReceived,
                       nodeId,
                       endpointId,
                       clusterName,
                       commandId,
                       commandName ) {}

}  // namespace Zigbee
}  // namespace HubPlatform
