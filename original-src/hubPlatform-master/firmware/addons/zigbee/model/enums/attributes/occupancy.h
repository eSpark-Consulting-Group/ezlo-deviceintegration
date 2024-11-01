#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Occupancy Sensing cluster, Occupancy attribute
enum class Occupancy {
    Unoccupied = 0x00,
    Occupied = 0x01,

    Unknown = 0xff
};

const char* toString( Occupancy id );
Occupancy occupancyFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
