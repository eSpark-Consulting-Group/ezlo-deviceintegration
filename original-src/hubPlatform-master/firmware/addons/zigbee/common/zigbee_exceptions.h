#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Zigbee {

class NodeNotFoundError : public std::runtime_error {
public:
    explicit NodeNotFoundError( uint16_t nodeId )
    : runtime_error{ "Node not found with id: " + std::to_string( nodeId ) } {}

    NodeNotFoundError( uint16_t nodeId, uint8_t endpointId )
    : runtime_error( "Node not found. Node id: " + std::to_string( nodeId ) +
                     ", endpoint id: " + std::to_string( endpointId ) ) {}

    NodeNotFoundError( uint16_t nodeId, uint8_t endpointId, uint16_t clusterId )
    : runtime_error( "Node not found. Node id: " + std::to_string( nodeId ) + ", endpoint id: " +
                     std::to_string( endpointId ) + ", cluster id: " + std::to_string( clusterId ) ) {}
};

}  // namespace Zigbee
}  // namespace HubPlatform
