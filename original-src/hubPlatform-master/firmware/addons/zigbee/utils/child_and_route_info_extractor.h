#pragma once

#include "zigbee/model/data_structures/child_and_route_structures.h"

#include <vector>
#include <string>

namespace HubPlatform {
namespace Zigbee {

class ZigbeeAddonCallbackInterface;

class ChildAndRouteInfoExtractor {
public:
    ChildAndRouteInfoExtractor() = delete;

    ChildAndRouteInfoExtractor( ZigbeeAddonCallbackInterface& callbackInterface );

    void extract();

private:
    void collectChildren( std::vector< ChildData >& result );
    void collectNeighbors( std::vector< NeighborData >& result );
    void collectroutes( std::vector< RouteData >& result );

    ZigbeeAddonCallbackInterface& m_callbackInterface;
};

}  // namespace Zigbee
}  // namespace HubPlatform
