#include "zigbee/model/enums/attributes/occupancy_sensor_type.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct OccupancySensorTypeData {
    OccupancySensorType id;
    const char* name;
};

static const OccupancySensorTypeData occupancySensorTypeData[] = {
    { OccupancySensorType::Pir,
      Resources::Fields::Attribute::Formats::OccupancySensingCluster::OccupancySensorType::Pir },
    { OccupancySensorType::Ultrasonic,
      Resources::Fields::Attribute::Formats::OccupancySensingCluster::OccupancySensorType::Ultrasonic },
    { OccupancySensorType::PirAndUltrasonic,
      Resources::Fields::Attribute::Formats::OccupancySensingCluster::OccupancySensorType::PirAndUltrasonic },
    { OccupancySensorType::PhysicalContact,
      Resources::Fields::Attribute::Formats::OccupancySensingCluster::OccupancySensorType::PhysicalContact },

};

const char* toString( OccupancySensorType id ) {
    for( const auto& data : occupancySensorTypeData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Attribute::Formats::Unknown;
}

OccupancySensorType occupancySensorTypeFromString( const std::string& name ) {
    for( const auto& data : occupancySensorTypeData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return OccupancySensorType::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
