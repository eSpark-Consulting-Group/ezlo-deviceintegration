#pragma once

#include "service_interfaces/script_event.h"
#include "zigbee/model/data_structures/child_and_route_structures.h"

namespace HubPlatform {
namespace Zigbee {

class ChildAndRouteInfoCollected : public Services::ScriptEvent {
public:
    explicit ChildAndRouteInfoCollected( uint16_t selfNodeId,
                                         std::vector< ChildData > children,
                                         std::vector< NeighborData > neighbors,
                                         std::vector< RouteData > routes );

public:
    Json toJson() const override;

private:
    uint16_t m_selfNodeId;
    std::vector< ChildData > m_children;
    std::vector< NeighborData > m_neighbors;
    std::vector< RouteData > m_routes;
};

}  // namespace Zigbee
}  // namespace HubPlatform
