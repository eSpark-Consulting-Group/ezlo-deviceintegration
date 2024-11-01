#pragma once

#include "zigbee/resources/zigbee_resources.h"

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class ClusterId {
    // General clusters
    Basic = 0x0000,
    PowerConfiguration = 0x0001,
    DeviceTemperatureConfiguration = 0x0002,
    Identify = 0x0003,
    Groups = 0x0004,
    Scenes = 0x0005,
    OnOff = 0x0006,
    OnOffSwitchConfiguration = 0x0007,
    LevelControl = 0x0008,
    Alarms = 0x0009,
    Time = 0x000A,
    RSSILocation = 0x000B,
    Diagnostics = 0x0B05,
    PollControl = 0x0020,
    PowerProfile = 0x001A,
    MeterIdentification = 0x0B01,
    ElectricalMeasurement = 0x0B04,
    AnalogInputBasic = 0x000C,
    AnalogOutputBasic = 0x000D,
    AnalogValueBasic = 0x000E,
    BinaryInputBasic = 0x000F,
    BinaryOutputBasic = 0x0010,
    BinaryValueBasic = 0x0011,
    MultistateInputBasic = 0x0012,
    MultistateOutputBasic = 0x0013,
    MultistateValueBasic = 0x0014,
    OtaUpgrade = 0x0019,
    DoorLock = 0x0101,
    Thermostat = 0x0201,
    FanControl = 0x0202,
    ColorControl = 0x0300,
    TemperatureMeasurement = 0x0402,
    IlluminanceMeasurement = 0x0400,
    RelativeHumidity = 0x0405,
    OccupancySensing = 0x0406,
    LeafWetness = 0x0407,
    SoilMoisture = 0x0408,
    // Security clusters
    IASZone = 0x0500,
    IASAce = 0x0501,
    IASWd = 0x0502,
    ApplianceEventsAndAlerts = 0x0B02,

    Unknown = 0xFFFF
};

const char* toString( ClusterId id );
ClusterId clusterIdFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
