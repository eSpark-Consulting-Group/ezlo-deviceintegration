#include "zigbee/utils/child_and_route_info_extractor.h"

#include "zigbee/zigbee_addon_callback.h"

extern "C" {
#include <Zigbee/definitions.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include "app/framework/util/af-main.h"
}

namespace HubPlatform {
namespace Zigbee {

ChildAndRouteInfoExtractor::ChildAndRouteInfoExtractor( ZigbeeAddonCallbackInterface& callbackInterface )
: m_callbackInterface( callbackInterface ) {}

void ChildAndRouteInfoExtractor::extract() {
    std::vector< ChildData > children;
    std::vector< NeighborData > neighborTableData;
    std::vector< RouteData > routeTableData;

    EmberNodeId selfId = emberAfGetNodeId();

    collectChildren( children );
    collectNeighbors( neighborTableData );
    collectroutes( routeTableData );

    m_callbackInterface.onChildAndRouteInfo(
        selfId, std::move( children ), std::move( neighborTableData ), std::move( routeTableData ) );
}

void ChildAndRouteInfoExtractor::collectChildren( std::vector< ChildData >& result ) {
    uint8_t size = emberAfGetChildTableSize();
    for( uint8_t i = 0; i < size; i++ ) {
        EmberChildData childData;
        EmberStatus status = emberAfGetChildData( i, &childData );
        if( status != EMBER_SUCCESS ) {
            continue;
        }

        ChildData convertedData{ childData.id, static_cast< ChildData::Type >( childData.type ), childData.timeout };
        result.push_back( std::move( convertedData ) );
    }
}

void ChildAndRouteInfoExtractor::collectNeighbors( std::vector< NeighborData >& result ) {
    for( uint8_t i = 0; i < emberAfGetNeighborTableSize(); i++ ) {
        EmberNeighborTableEntry neighborData;

        EmberStatus status = emberGetNeighbor( i, &neighborData );
        if( ( status != EMBER_SUCCESS ) || ( neighborData.shortId == EMBER_NULL_NODE_ID ) ) {
            continue;
        }

        NeighborData convertedData{
            neighborData.shortId, neighborData.averageLqi, neighborData.age, neighborData.inCost, neighborData.outCost
        };
        result.push_back( std::move( convertedData ) );
    }
}

void ChildAndRouteInfoExtractor::collectroutes( std::vector< RouteData >& result ) {
    for( uint8_t i = 0; i < emberAfGetRouteTableSize(); i++ ) {
        EmberRouteTableEntry entry;
        if( EMBER_SUCCESS != emberGetRouteTableEntry( i, &entry ) || entry.destination == EMBER_NULL_NODE_ID ) {
            continue;
        }

        RouteData convertedData{ entry.destination,
                                 entry.nextHop,
                                 entry.age,
                                 static_cast< RouteData::ConcentratorType >( entry.concentratorType ),
                                 static_cast< RouteData::Status >( entry.status ) };
        result.push_back( std::move( convertedData ) );
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
