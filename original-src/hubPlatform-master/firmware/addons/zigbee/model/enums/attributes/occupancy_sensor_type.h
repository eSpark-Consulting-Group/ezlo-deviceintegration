#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Occupancy Sensing cluster, OccupancySensorType and OccupancySensorTypeBitmap attribute
enum class OccupancySensorType {
    Pir = 0x00,
    Ultrasonic = 0x01,
    PirAndUltrasonic = 0x02,
    PhysicalContact = 0x03,

    Unknown = 0xff
};

const char* toString( OccupancySensorType id );
OccupancySensorType occupancySensorTypeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
