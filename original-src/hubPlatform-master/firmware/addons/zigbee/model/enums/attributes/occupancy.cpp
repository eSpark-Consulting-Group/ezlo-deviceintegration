#include "zigbee/model/enums/attributes/occupancy.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct OccupancyData {
    Occupancy id;
    const char* name;
};

static const OccupancyData occupancyData[] = {
    { Occupancy::Unoccupied, Resources::Fields::Attribute::Formats::OccupancySensingCluster::Occupancy::Unoccupied },
    { Occupancy::Occupied, Resources::Fields::Attribute::Formats::OccupancySensingCluster::Occupancy::Occupied },

};

const char* toString( Occupancy id ) {
    for( const auto& data : occupancyData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Attribute::Formats::Unknown;
}

Occupancy occupancyFromString( const std::string& name ) {
    for( const auto& data : occupancyData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return Occupancy::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
