#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

struct ClusterIdData {
    ClusterId clusterId;
    const char* name;
};

static const ClusterIdData clusterIdData[] = {
    { ClusterId::Basic, Resources::Fields::Cluster::Names::Basic },
    { ClusterId::PowerConfiguration, Resources::Fields::Cluster::Names::PowerConfiguration },
    { ClusterId::DeviceTemperatureConfiguration, Resources::Fields::Cluster::Names::DeviceTemperatureConfiguration },
    { ClusterId::Identify, Resources::Fields::Cluster::Names::Identify },
    { ClusterId::Groups, Resources::Fields::Cluster::Names::Groups },
    { ClusterId::Scenes, Resources::Fields::Cluster::Names::Scenes },
    { ClusterId::OnOff, Resources::Fields::Cluster::Names::OnOff },
    { ClusterId::OnOffSwitchConfiguration, Resources::Fields::Cluster::Names::OnOffSwitchConfiguration },
    { ClusterId::LevelControl, Resources::Fields::Cluster::Names::LevelControl },
    { ClusterId::Alarms, Resources::Fields::Cluster::Names::Alarms },
    { ClusterId::Time, Resources::Fields::Cluster::Names::Time },
    { ClusterId::RSSILocation, Resources::Fields::Cluster::Names::RSSILocation },
    { ClusterId::Diagnostics, Resources::Fields::Cluster::Names::Diagnostics },
    { ClusterId::PollControl, Resources::Fields::Cluster::Names::PollControl },
    { ClusterId::PowerProfile, Resources::Fields::Cluster::Names::PowerProfile },
    { ClusterId::MeterIdentification, Resources::Fields::Cluster::Names::MeterIdentification },
    { ClusterId::ElectricalMeasurement, Resources::Fields::Cluster::Names::ElectricalMeasurement },
    { ClusterId::AnalogInputBasic, Resources::Fields::Cluster::Names::AnalogInputBasic },
    { ClusterId::AnalogOutputBasic, Resources::Fields::Cluster::Names::AnalogOutputBasic },
    { ClusterId::AnalogValueBasic, Resources::Fields::Cluster::Names::AnalogValueBasic },
    { ClusterId::BinaryInputBasic, Resources::Fields::Cluster::Names::BinaryInputBasic },
    { ClusterId::BinaryOutputBasic, Resources::Fields::Cluster::Names::BinaryOutputBasic },
    { ClusterId::BinaryValueBasic, Resources::Fields::Cluster::Names::BinaryValueBasic },
    { ClusterId::MultistateInputBasic, Resources::Fields::Cluster::Names::MultistateInputBasic },
    { ClusterId::MultistateOutputBasic, Resources::Fields::Cluster::Names::MultistateOutputBasic },
    { ClusterId::MultistateValueBasic, Resources::Fields::Cluster::Names::MultistateValueBasic },
    { ClusterId::OtaUpgrade, Resources::Fields::Cluster::Names::OtaUpgrade },
    { ClusterId::DoorLock, Resources::Fields::Cluster::Names::DoorLock },
    { ClusterId::Thermostat, Resources::Fields::Cluster::Names::Thermostat },
    { ClusterId::FanControl, Resources::Fields::Cluster::Names::FanControl },
    { ClusterId::ColorControl, Resources::Fields::Cluster::Names::ColorControl },
    { ClusterId::IlluminanceMeasurement, Resources::Fields::Cluster::Names::IlluminanceMeasurement },
    { ClusterId::TemperatureMeasurement, Resources::Fields::Cluster::Names::TemperatureMeasurement },
    { ClusterId::RelativeHumidity, Resources::Fields::Cluster::Names::RelativeHumidity },
    { ClusterId::OccupancySensing, Resources::Fields::Cluster::Names::OccupancySensing },
    { ClusterId::LeafWetness, Resources::Fields::Cluster::Names::LeafWetness },
    { ClusterId::SoilMoisture, Resources::Fields::Cluster::Names::SoilMoisture },
    { ClusterId::ApplianceEventsAndAlerts, Resources::Fields::Cluster::Names::ApplianceEventsAndAlerts },

    { ClusterId::IASZone, Resources::Fields::Cluster::Names::IASZone },
    { ClusterId::IASAce, Resources::Fields::Cluster::Names::IASAce },
    { ClusterId::IASWd, Resources::Fields::Cluster::Names::IASWd }
};

const char* toString( ClusterId id ) {
    for( const auto& data : clusterIdData ) {
        if( data.clusterId == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Cluster::Names::Unknown;
}

ClusterId clusterIdFromString( const std::string& name ) {
    for( const auto& data : clusterIdData ) {
        if( name == data.name ) {
            return data.clusterId;
        }
    }

    return ClusterId::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
