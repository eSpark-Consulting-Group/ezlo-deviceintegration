#include "zigbee/script_events/child_and_route_info_collected.h"
#include "zigbee/resources/zigbee_resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Zigbee {

ChildAndRouteInfoCollected::ChildAndRouteInfoCollected( uint16_t selfNodeId,
                                                        std::vector< ChildData > children,
                                                        std::vector< NeighborData > neighbors,
                                                        std::vector< RouteData > routes )
: Services::ScriptEvent( Resources::ScriptEvent::Name::ChildAndRouteInfoCollected )
, m_selfNodeId( selfNodeId )
, m_children( std::move( children ) )
, m_neighbors( std::move( neighbors ) )
, m_routes( std::move( routes ) ) {}

Json ChildAndRouteInfoCollected::toJson() const {
    using namespace Resources::ScriptEvent::Param;

    auto result = Services::ScriptEvent::toJson();

    result[ ChildAndRouteInfo::ControllerNodeId ] = m_selfNodeId;

    JsonArray children;
    for( const auto& child : m_children ) {
        children.push_back( JSON( NodeId << child.id << ChildAndRouteInfo::Type << child.typeToString()
                                         << ChildAndRouteInfo::Timeout << child.timeoutToString() ) );
    }

    result[ ChildAndRouteInfo::Children ] = std::move( children );

    JsonArray neighbors;
    for( const auto& neighbor : m_neighbors ) {
        neighbors.push_back( JSON( NodeId << neighbor.id << ChildAndRouteInfo::Lqi << neighbor.averageLqi
                                          << ChildAndRouteInfo::Age << neighbor.age << ChildAndRouteInfo::InCost
                                          << neighbor.inCost << ChildAndRouteInfo::OutCost << neighbor.outCost ) );
    }

    result[ ChildAndRouteInfo::Neighbors ] = std::move( neighbors );

    JsonArray routes;
    for( const auto& route : m_routes ) {
        routes.push_back( JSON( ChildAndRouteInfo::To
                                << route.destination << ChildAndRouteInfo::Through << route.nextHop
                                << ChildAndRouteInfo::Age << route.age << ChildAndRouteInfo::ConcentratorType
                                << route.concentratorTypeToString() << Status << route.statusToString() ) );
    }

    result[ ChildAndRouteInfo::Routes ] = std::move( routes );

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
