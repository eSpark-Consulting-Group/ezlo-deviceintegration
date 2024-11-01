#pragma once

#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/attribute_id.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Zigbee {

class AttributesFormatter {
public:
    AttributesFormatter() = delete;

    static Json formatAttributes( uint16_t id, uint16_t clusterId, const std::string& value );

private:
    static Json formatIASZoneAttribute( uint16_t id, const std::string& value );

    static Json formatElectricalMeasurementAttribute( uint16_t id, const std::string& value );

    static Json formatTemperatureMeasurementAttribute( uint16_t id, const std::string& value );

    static Json formatIlluminanceMeasurementAttribute( uint16_t id, const std::string& value );

    static Json formatWaterContentAttribute( uint16_t id, const std::string& value );

    static Json formatThermostatAttribute( uint16_t id, const std::string& value );

    static Json formatFanControlAttribute( uint16_t id, const std::string& value );

    static Json formatPowerConfigurationAttribute( uint16_t id, const std::string& value );

    static Json formatLevelControlAttribute( uint16_t id, const std::string& value );

    static Json formatColorControlAttribute( uint16_t id, const std::string& value );

    static Json formatDoorLockAttribute( uint16_t id, const std::string& value );

    static Json formatOccupancySensingAttribute( uint16_t id, const std::string& value );
};

}  // namespace Zigbee
}  // namespace HubPlatform
