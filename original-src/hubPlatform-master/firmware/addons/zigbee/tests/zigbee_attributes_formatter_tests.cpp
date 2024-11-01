#include "zigbee/utils/attributes_formatter.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

    TEST PLAN
DONE    1. Unsupported cluster
DONE    2. IAS Zone cluster
DONE        2.1 ZoneState
DONE        2.2 ZoneType
DONE        2.3 ZoneStatus
DONE    3. Electrical Measurement cluster
DONE        3.1 RMSVoltage
DONE        3.2 RMSCurrent
DONE        3.3 ActivePower
DONE        3.4 ACVoltageMultiplier
DONE        3.5 ACVoltageDivisor
DONE        3.6 ACCurrentMultiplier
DONE        3.7 ACCurrentDivisor
DONE        3.8 ACPowerMultiplier
DONE        3.9 ACPowerDivisor
DONE    4. Temperature measurement cluster
DONE        4.1 MeasuredValue
DONE        4.2 MinMeasuredValue
DONE        4.3 MaxMeasuredValue
DONE        4.4 Tolerance
DONE    5. Illuminance measurement cluster
DONE        5.1 MeasuredValue
DONE        5.1 MinMeasuredValue
DONE        5.1 MaxMeasuredValue
DONE    6. Water content clusters
DONE        6.1 MeasuredValue
DONE        6.2 MinMeasuredValue
DONE        6.3 MaxMeasuredValue
DONE        6.4 Tolerance
DONE    7. Thermostat cluster
DONE        7.1 LocalTemperature
DONE        7.2 OccupiedCoolingSetpoint
DONE        7.3 OccupiedHeatingSetpoint
DONE        7.4 MinHeatSetpointLimit
DONE        7.5 MaxHeatSetpointLimit
DONE        7.6 MinCoolSetpointLimit
DONE        7.7 MaxCoolSetpointLimit
DONE        7.8 MinSetpointDeadBand
DONE        7.9 LocalTemperatureCalibration
DONE        7.10 SystemMode
DONE        7.11 ThermostatRunningState
DONE    8. Fan control cluster
DONE        8.1 FanMode
DONE        8.2 FanModeSequence
DONE    9. Power configuration cluster
DONE        9.1 BatteryVoltage
DONE        9.2 BatteryPercentageRemaining
DONE        9.3 BatteryQuantity
DONE        9.4 BatterySize
DONE    10. Level control cluster
DONE        10.1 CurrentLevel
DONE    11. Color control cluster
DONE        11.1 CurrentX
DONE        11.2 CurrentY
DONE        11.3 ColorTempPhysicalMinMireds
DONE        11.4 ColorTempPhysicalMaxMireds
DONE        11.5 ColorTemperatureMireds
DONE        11.6 ColorMode
DONE        11.7 ColorCapabilities
DONE    12. Door lock cluster
DONE        12.1 LockState
DONE        12.2 LockType
DONE        12.3 ActuatorEnabled
DONE        12.4 DoorState
DONE        12.5 DoorOpenEvents
DONE        12.6 DoorClosedEvents
DONE        12.7 OpenPeriod
DONE        12.8 NumberOfTotalUsersSupported
DONE        12.9 NumberOfPINUsersSupported
DONE        12.10 NumberOfRFIDUsersSupported
DONE        12.11 MaxPINCodeLength
DONE        12.12 MinPINCodeLength
DONE        12.13 MaxRFIDCodeLength
DONE        12.14 MinRFIDCodeLength
DONE    13. Occupancy Sensing cluster
DONE        13.1 Occupancy
DONE        13.2 OccupancySensorType
DONE        13.3 OccupancySensorTypeBitmap
DONE        13.4 PIROccupiedToUnoccupiedDelay
DONE        13.5 PIRUnoccupiedToOccupiedDelay
DONE        13.6 PIRUnoccupiedToOccupiedThreshold
DONE        13.7 UltrasonicOccupiedToUnoccupiedDelay
DONE        13.8 UltrasonicUnoccupiedToOccupiedDelay
DONE        13.9 UltrasonicUnoccupiedToOccupiedThreshold
DONE        13.10 PhysicalContactOccupiedToUnoccupiedDelay
DONE        13.11 PhysicalContactUnoccupiedToOccupiedDelay
DONE        13.12 PhysicalContactUnoccupiedToOccupiedThreshold

 */

namespace HubPlatform {
namespace Zigbee {
namespace UT {
using namespace Resources::ScriptEvent;
using namespace ::testing;

class ZigbeeAttributesFormatter : public Test {
public:
    void testUnsignedInt( uint16_t attributeId, uint16_t clusterId, std::string undefinedValue = std::string() ) {
        EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, "0" ), 0 );
        EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, "15" ), 15 );

        if( !undefinedValue.empty() ) {
            EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, undefinedValue ), "undefined" );
        }
    }

    void testInt( uint16_t attributeId, uint16_t clusterId, std::string undefinedValue = std::string() ) {
        EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, "0" ), 0 );
        EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, "15" ), 15 );
        EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, "-15" ), -15 );

        if( !undefinedValue.empty() ) {
            EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, undefinedValue ), "undefined" );
        }
    }

    void testSeveral( uint16_t attributeId,
                      uint16_t clusterId,
                      const std::vector< std::pair< std::string, Json > >& values ) {
        for( int i = 0; i < values.size(); ++i ) {
            EXPECT_EQ( AttributesFormatter::formatAttributes( attributeId, clusterId, values.at( i ).first ),
                       values.at( i ).second );
        }
    }
};

TEST_F( ZigbeeAttributesFormatter, unsupportedCluster_1 ) {
    EXPECT_EQ( AttributesFormatter::formatAttributes( 0x00, 0xffff, "test" ), "test" );
}

TEST_F( ZigbeeAttributesFormatter, iasZoneClusterZoneState_2_1 ) {
    testSeveral( 0x0000, 0x0500, { { "0", "not_entrolled" }, { "1", "enrolled" }, { "2", "unknown" } } );
}

TEST_F( ZigbeeAttributesFormatter, iasZoneClusterZoneType_2_2 ) {
    auto generateResult = []( std::string zoneType, std::string alarm1, std::string alarm2 = std::string() ) -> Json {
        Json result;
        result[ "zone_type" ] = zoneType;
        result[ "alarm_1" ] = alarm1;
        if( !alarm2.empty() ) {
            result[ "alarm_2" ] = alarm2;
        }

        return result;
    };

    testSeveral( 0x0001,
                 0x0500,
                 {
                     { "0", generateResult( "standard_cie", "system_alarm" ) },
                     { "13", generateResult( "motion_sensor", "intrusion_indication", "presence_indication" ) },
                     { "21", generateResult( "contact_switch", "first_portal", "second_portal" ) },
                     { "40", generateResult( "fire_sensor", "fire_indication" ) },
                     { "42", generateResult( "water_sensor", "water_overflow" ) },
                     { "43", generateResult( "gas_sensor", "co_indication", "coocing_indication" ) },
                     { "44", generateResult( "personal_emergency_device", "fall", "button" ) },
                     { "45", generateResult( "vibration_movement_sensor", "movement_indication", "vibration" ) },
                     { "271", generateResult( "remote_control", "panic", "emergency" ) },
                     { "277", generateResult( "key_fob", "panic", "emergency" ) },
                     { "541", generateResult( "keypad", "panic", "emergency" ) },
                     { "1", "unknown" },
                 } );
}

TEST_F( ZigbeeAttributesFormatter, iasZoneClusterZoneStatus_2_3 ) {
    auto defaultStatus = JSON( "alarm_1"
                               << "off"
                               << "alarm_2"
                               << "off"
                               << "tamper"
                               << "not_tampered"
                               << "battery"
                               << "ok"
                               << "supervision_reports"
                               << "no"
                               << "restore_reports"
                               << "no"
                               << "trouble"
                               << "ok"
                               << "ac"
                               << "ok" );

    auto getChangedStatus = [&defaultStatus]( std::string field, std::string value ) -> Json {
        Json result = defaultStatus;
        result[ field ] = value;

        return result;
    };

    testSeveral( 0x0002,
                 0x0500,
                 { { "0", defaultStatus },
                   { "1", getChangedStatus( "alarm_1", "on" ) },
                   { "2", getChangedStatus( "alarm_2", "on" ) },
                   { "4", getChangedStatus( "tamper", "tampered" ) },
                   { "8", getChangedStatus( "battery", "low" ) },
                   { "16", getChangedStatus( "supervision_reports", "yes" ) },
                   { "32", getChangedStatus( "restore_reports", "yes" ) },
                   { "64", getChangedStatus( "trouble", "trouble" ) },
                   { "128", getChangedStatus( "ac", "fault" ) } } );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterRMSVoltage_3_1 ) {
    testUnsignedInt( 0x0505, 0x0B04, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterRMSCurrent_3_2 ) {
    testUnsignedInt( 0x0508, 0x0B04, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterActivePower_3_3 ) {
    testInt( 0x050B, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterACVoltageMultiplier_3_4 ) {
    testUnsignedInt( 0x0600, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterACVoltageDivisor_3_5 ) {
    testUnsignedInt( 0x0601, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterACCurrentMultiplier_3_6 ) {
    testUnsignedInt( 0x0602, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterACCurrentDivisor_3_7 ) {
    testUnsignedInt( 0x0603, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterACPowerMultiplier_3_8 ) {
    testUnsignedInt( 0x0604, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, electricalMeasurementClusterACPowerDivisor_3_9 ) {
    testUnsignedInt( 0x0605, 0x0B04 );
}

TEST_F( ZigbeeAttributesFormatter, temperatureMeasurementClusterMeasuredValue_4_1 ) {
    testInt( 0x0000, 0x0402, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, temperatureMeasurementClusterMinMeasuredValue_4_2 ) {
    testInt( 0x0001, 0x0402, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, temperatureMeasurementClusterMaxMeasuredValue_4_3 ) {
    testInt( 0x0002, 0x0402, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, temperatureMeasurementClusterTolerance_4_4 ) {
    testUnsignedInt( 0x0003, 0x0402 );
}

TEST_F( ZigbeeAttributesFormatter, illuminanceMeasurementClusterMeasuredValue_5_1 ) {
    testUnsignedInt( 0x0000, 0x0400, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, illuminanceMeasurementClusterMinMeasuredValue_5_2 ) {
    testUnsignedInt( 0x0001, 0x0400, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, illuminanceMeasurementClusterMaxMeasuredValue_5_3 ) {
    testUnsignedInt( 0x0002, 0x0400, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, waterContentClustersMeasuredValue_6_1 ) {
    testUnsignedInt( 0x0000, 0x0405, "65535" );
    testUnsignedInt( 0x0000, 0x0407, "65535" );
    testUnsignedInt( 0x0000, 0x0408, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, waterContentClustersMinMeasuredValue_6_2 ) {
    testUnsignedInt( 0x0001, 0x0405, "65535" );
    testUnsignedInt( 0x0001, 0x0407, "65535" );
    testUnsignedInt( 0x0001, 0x0408, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, waterContentClustersMaxMeasuredValue_6_3 ) {
    testUnsignedInt( 0x0002, 0x0405, "65535" );
    testUnsignedInt( 0x0002, 0x0407, "65535" );
    testUnsignedInt( 0x0002, 0x0408, "65535" );
}

TEST_F( ZigbeeAttributesFormatter, waterContentClustersTolerance_6_4 ) {
    testUnsignedInt( 0x0003, 0x0405 );
    testUnsignedInt( 0x0003, 0x0407 );
    testUnsignedInt( 0x0003, 0x0408 );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterLocalTemperature_7_1 ) {
    testInt( 0x0000, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterOccupiedCoolingSetpoint_7_2 ) {
    testInt( 0x0011, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterOccupiedHeatingSetpoint_7_3 ) {
    testInt( 0x0012, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterMinHeatSetpointLimit_7_4 ) {
    testInt( 0x0015, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterMaxHeatSetpointLimit_7_5 ) {
    testInt( 0x0016, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterMinCoolSetpointLimit_7_6 ) {
    testInt( 0x0017, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterMaxCoolSetpointLimit_7_7 ) {
    testInt( 0x0018, 0x0201, "-32768" );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterMinSetpointDeadBand_7_8 ) {
    testInt( 0x0019, 0x0201 );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterLocalTemperatureCalibration_7_9 ) {
    testInt( 0x0010, 0x0201 );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterSystemMode_7_10 ) {
    testSeveral( 0x001c,
                 0x0201,
                 {
                     { "0", "off" },
                     { "1", "auto" },
                     { "3", "cool" },
                     { "4", "heat" },
                     { "5", "emergency_heating" },
                     { "6", "precooling" },
                     { "7", "fan_only" },
                     { "8", "dry" },
                     { "9", "sleep" },
                 } );
}

TEST_F( ZigbeeAttributesFormatter, thermostatClusterThermostatRunningState_7_11 ) {
    testSeveral( 0x0029,
                 0x0201,
                 { { "0", JsonArray() },
                   { "1", JSON_ARRAY( "heat" ) },
                   { "2", JSON_ARRAY( "cool" ) },
                   { "4", JSON_ARRAY( "fan" ) },
                   { "8", JSON_ARRAY( "heat_2nd_stage" ) },
                   { "16", JSON_ARRAY( "cool_2nd_stage" ) },
                   { "32", JSON_ARRAY( "fan_2nd_stage" ) },
                   { "64", JSON_ARRAY( "fan_3rd_stage" ) },

                   { "127",
                     JSON_ARRAY( "heat"
                                 << "cool"
                                 << "fan"
                                 << "heat_2nd_stage"
                                 << "cool_2nd_stage"
                                 << "fan_2nd_stage"
                                 << "fan_3rd_stage" ) }

                 } );
}

TEST_F( ZigbeeAttributesFormatter, fanControlClusterFanMode_8_1 ) {
    testSeveral(
        0x0000,
        0x0202,
        { { "0", "off" }, { "1", "low" }, { "2", "medium" }, { "3", "high" }, { "4", "on" }, { "5", "auto" } } );
}

TEST_F( ZigbeeAttributesFormatter, fanControlClusterFanModeSequence_8_2 ) {
    testSeveral( 0x0001,
                 0x0202,
                 { { "0",
                     JSON_ARRAY( "low"
                                 << "medium"
                                 << "high" ) },
                   { "1",
                     JSON_ARRAY( "low"
                                 << "high" ) },
                   { "2",
                     JSON_ARRAY( "low"
                                 << "medium"
                                 << "high"
                                 << "auto" ) },
                   { "3",
                     JSON_ARRAY( "low"
                                 << "high"
                                 << "auto" ) },
                   { "4",
                     JSON_ARRAY( "on"
                                 << "auto" ) },
                   { "5", JsonArray() } } );
}

TEST_F( ZigbeeAttributesFormatter, powerConfigurationClusterBatteryVoltage_9_1 ) {
    testUnsignedInt( 0x0020, 0x0001, "255" );
}

TEST_F( ZigbeeAttributesFormatter, powerConfigurationClusterBatteryPercentageRemaining_9_2 ) {
    testUnsignedInt( 0x0021, 0x0001, "255" );
}

TEST_F( ZigbeeAttributesFormatter, powerConfigurationClusterBatteryQuantity_9_3 ) {
    testUnsignedInt( 0x0033, 0x0001 );
}

TEST_F( ZigbeeAttributesFormatter, powerConfigurationClusterBatterySize_9_4 ) {
    testSeveral( 0x0031,
                 0x0001,
                 { { "0", "no_battery" },
                   { "1", "built_in" },
                   { "2", "other" },
                   { "3", "AA" },
                   { "4", "AAA" },
                   { "5", "C" },
                   { "6", "D" },
                   { "7", "CR2" },
                   { "8", "CR123A" } } );
}

TEST_F( ZigbeeAttributesFormatter, levelControlClusterColorCurrentLevel_10_1 ) {
    testUnsignedInt( 0x0000, 0x0008 );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterCurrentX_11_1 ) {
    testUnsignedInt( 0x03, 0x0300 );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterCurrentY_11_2 ) {
    testUnsignedInt( 0x04, 0x0300 );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterColorTempPhysicalMinMireds_11_3 ) {
    testUnsignedInt( 0x400b, 0x0300 );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterColorTempPhysicalMaxMireds_11_4 ) {
    testUnsignedInt( 0x400c, 0x0300 );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterColorTemperatureMireds_11_5 ) {
    testUnsignedInt( 0x0007, 0x0300 );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterColorMode_11_6 ) {
    testSeveral(
        0x0008, 0x0300, { { "0", "color_hue_saturation" }, { "1", "color_xy" }, { "2", "color_temperature" } } );
}

TEST_F( ZigbeeAttributesFormatter, colorControlClusterColorCapabilities_11_7 ) {
    testSeveral( 0x400a,
                 0x0300,
                 { { "0", JsonArray() },
                   { "1", JSON_ARRAY( "hue_saturation_supported" ) },
                   { "2", JSON_ARRAY( "enhanced_hue_supported" ) },
                   { "4", JSON_ARRAY( "color_loop_supported" ) },
                   { "8", JSON_ARRAY( "xy_attributes_supported" ) },
                   { "16", JSON_ARRAY( "color_temperature_supported" ) },

                   { "3",
                     JSON_ARRAY( "hue_saturation_supported"
                                 << "enhanced_hue_supported" ) },
                   { "5",
                     JSON_ARRAY( "hue_saturation_supported"
                                 << "color_loop_supported" ) },
                   { "9",
                     JSON_ARRAY( "hue_saturation_supported"
                                 << "xy_attributes_supported" ) },
                   { "17",
                     JSON_ARRAY( "hue_saturation_supported"
                                 << "color_temperature_supported" ) },

                   { "6",
                     JSON_ARRAY( "enhanced_hue_supported"
                                 << "color_loop_supported" ) },
                   { "10",
                     JSON_ARRAY( "enhanced_hue_supported"
                                 << "xy_attributes_supported" ) },
                   { "18",
                     JSON_ARRAY( "enhanced_hue_supported"
                                 << "color_temperature_supported" ) },

                   { "12",
                     JSON_ARRAY( "color_loop_supported"
                                 << "xy_attributes_supported" ) },
                   { "20",
                     JSON_ARRAY( "color_loop_supported"
                                 << "color_temperature_supported" ) },

                   { "24",
                     JSON_ARRAY( "xy_attributes_supported"
                                 << "color_temperature_supported" ) },

                   { "31",
                     JSON_ARRAY( "hue_saturation_supported"
                                 << "enhanced_hue_supported"
                                 << "color_loop_supported"
                                 << "xy_attributes_supported"
                                 << "color_temperature_supported" ) }

                 } );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterLockState_12_1 ) {
    testSeveral(
        0x0000, 0x0101, { { "0", "not_fully_locked" }, { "1", "locked" }, { "2", "unlocked" }, { "255", "unknown" } } );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterLockType_12_2 ) {
    testSeveral( 0x0001,
                 0x0101,
                 { { "0", "dead_bolt" },
                   { "1", "magnetic" },
                   { "2", "other" },
                   { "3", "mortise" },
                   { "4", "rim" },
                   { "5", "latch_bolt" },
                   { "6", "cylindrical_lock" },
                   { "7", "tubular_lock" },
                   { "8", "interconnected_lock" },
                   { "9", "dead_latch" },
                   { "10", "door_furniture" } } );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterActuatorEnabled_12_3 ) {
    testSeveral( 0x0002, 0x0101, { { "false", "disabled" }, { "true", "enabled" } } );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterDoorState_12_4 ) {
    testSeveral( 0x0003,
                 0x0101,
                 { { "0", "open" },
                   { "1", "closed" },
                   { "2", "error_jammed" },
                   { "3", "error_forced_open" },
                   { "4", "error_unspecified" },
                   { "255", "unknown" } } );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterDoorOpenEvents_12_5 ) {
    testUnsignedInt( 0x0004, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterDoorClosedEvents_12_6 ) {
    testUnsignedInt( 0x0005, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterOpenPeriod_12_7 ) {
    testUnsignedInt( 0x0006, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterNumberOfTotalUsersSupported_12_8 ) {
    testUnsignedInt( 0x0011, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterNumberOfPINUsersSupported_12_9 ) {
    testUnsignedInt( 0x0012, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterNumberOfRFIDUsersSupported_12_10 ) {
    testUnsignedInt( 0x0013, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterMaxPINCodeLength_12_11 ) {
    testUnsignedInt( 0x0017, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterMinPINCodeLength_12_12 ) {
    testUnsignedInt( 0x0018, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterMaxRFIDCodeLength_12_13 ) {
    testUnsignedInt( 0x0019, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, doorLockClusterMinRFIDCodeLength_12_14 ) {
    testUnsignedInt( 0x001A, 0x0101 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterOccupancy_13_1 ) {
    testSeveral(
        0x0000, 0x0406, { { "0", "unoccupied" }, { "1", "occupied" }, { "2", "unknown" }, { "255", "unknown" } } );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterOccupancySensorType_13_2 ) {
    testSeveral( 0x0001,
                 0x0406,
                 { { "0", "pir" },
                   { "1", "ultrasonic" },
                   { "2", "pir_and_ultrasonic" },
                   { "3", "physical_contact" },
                   { "4", "unknown" },
                   { "255", "unknown" } } );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterOccupancySensorTypeBitmap_13_3 ) {
    testSeveral( 0x0002,
                 0x0406,
                 { { "0", JsonArray() },
                   { "1", JSON_ARRAY( "pir" ) },
                   { "2", JSON_ARRAY( "ultrasonic" ) },
                   { "4", JSON_ARRAY( "physical_contact" ) },

                   { "3",
                     JSON_ARRAY( "pir"
                                 << "ultrasonic" ) },
                   { "5",
                     JSON_ARRAY( "pir"
                                 << "physical_contact" ) },
                   { "6",
                     JSON_ARRAY( "ultrasonic"
                                 << "physical_contact" ) },
                   { "7",
                     JSON_ARRAY( "pir"
                                 << "ultrasonic"
                                 << "physical_contact" ) } } );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterPIROccupiedToUnoccupiedDelay_13_4 ) {
    testUnsignedInt( 0x0010, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterPIRUnoccupiedToOccupiedDelay_13_5 ) {
    testUnsignedInt( 0x0011, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterPIRUnoccupiedToOccupiedThreshold_13_6 ) {
    testUnsignedInt( 0x0012, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterUltrasonicOccupiedToUnoccupiedDelay_13_7 ) {
    testUnsignedInt( 0x0020, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterUltrasonicUnoccupiedToOccupiedDelay_13_8 ) {
    testUnsignedInt( 0x0021, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterUltrasonicUnoccupiedToOccupiedThreshold_13_9 ) {
    testUnsignedInt( 0x0022, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterPhysicalContactOccupiedToUnoccupiedDelay_13_10 ) {
    testUnsignedInt( 0x0030, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterPhysicalContactUnoccupiedToOccupiedDelay_13_11 ) {
    testUnsignedInt( 0x0031, 0x0406 );
}

TEST_F( ZigbeeAttributesFormatter, occupancySensingClusterPhysicalContactUnoccupiedToOccupiedThreshold_13_12 ) {
    testUnsignedInt( 0x0032, 0x0406 );
}

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
