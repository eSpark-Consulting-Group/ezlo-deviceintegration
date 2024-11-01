#include "zigbee/utils/attributes_formatter.h"
#include "zigbee/resources/zigbee_resources.h"

#include "zigbee/model/enums/attributes/thermostat_system_mode.h"
#include "zigbee/model/enums/attributes/thermostat_fan_mode.h"
#include "zigbee/model/enums/attributes/power_conf_battery_size.h"
#include "zigbee/model/enums/attributes/color_mode.h"
#include "zigbee/model/enums/attributes/color_capabilities.h"
#include "zigbee/model/enums/attributes/lock_state.h"
#include "zigbee/model/enums/attributes/lock_type.h"
#include "zigbee/model/enums/attributes/door_lock_actuator_state.h"
#include "zigbee/model/enums/attributes/door_state.h"
#include "zigbee/model/enums/attributes/occupancy.h"
#include "zigbee/model/enums/attributes/occupancy_sensor_type.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

Json AttributesFormatter::formatAttributes( uint16_t id, uint16_t clusterId, const std::string& value ) {
    switch( static_cast< ClusterId >( clusterId ) ) {
    case ClusterId::IASZone:
        return formatIASZoneAttribute( id, value );
    case ClusterId::ElectricalMeasurement:
        return formatElectricalMeasurementAttribute( id, value );
    case ClusterId::TemperatureMeasurement:
        return formatTemperatureMeasurementAttribute( id, value );
    case ClusterId::IlluminanceMeasurement:
        return formatIlluminanceMeasurementAttribute( id, value );
    case ClusterId::RelativeHumidity:
    case ClusterId::LeafWetness:
    case ClusterId::SoilMoisture:
        return formatWaterContentAttribute( id, value );
    case ClusterId::Thermostat:
        return formatThermostatAttribute( id, value );
    case ClusterId::FanControl:
        return formatFanControlAttribute( id, value );
    case ClusterId::PowerConfiguration:
        return formatPowerConfigurationAttribute( id, value );
    case ClusterId::LevelControl:
        return formatLevelControlAttribute( id, value );
    case ClusterId::ColorControl:
        return formatColorControlAttribute( id, value );
    case ClusterId::DoorLock:
        return formatDoorLockAttribute( id, value );
    case ClusterId::OccupancySensing:
        return formatOccupancySensingAttribute( id, value );
    default:
        return value;
    }
}

Json AttributesFormatter::formatIASZoneAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< IASZoneClusterAttributeId >( id ) ) {
    case IASZoneClusterAttributeId::ZoneState: {
        uint8_t intValue = std::stoul( value );
        switch( intValue ) {
        case 0:
            result = IASZoneCluster::ZoneState::NotEnrolled;
            break;
        case 1:
            result = IASZoneCluster::ZoneState::Enrolled;
            break;
        default:
            result = Unknown;
        }
    } break;

    case IASZoneClusterAttributeId::ZoneType: {
        uint16_t intValue = std::stoul( value );
        switch( intValue ) {
        case 0x0000:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::StandardCie;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::SystemAlarm;
            break;

        case 0x000d:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::MotionSensor;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::IntrusionIndication;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::PresenceIndication;
            break;

        case 0x0015:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::ContactSwitch;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::FirstPortal;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::SecondPortal;
            break;

        case 0x0028:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::FireSensor;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::FireIndication;
            break;

        case 0x002a:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::WaterSensor;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::WaterOverflow;
            break;

        case 0x002b:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::GasSensor;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::CoIndication;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::CoocingIndication;
            break;

        case 0x002c:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::PersonalEmergencyDevice;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::Fall;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::Button;
            break;

        case 0x002d:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::VibrationMovementSensor;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::MovementIndication;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::Vibration;
            break;

        case 0x010f:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::RemoteControl;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::Panic;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::Emergency;
            break;

        case 0x0115:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::KeyFob;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::Panic;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::Emergency;
            break;

        case 0x021d:
            result[ IASZoneCluster::ZoneType::ZoneType ] = IASZoneCluster::ZoneType::Keypad;
            result[ IASZoneCluster::Alarm1 ] = IASZoneCluster::ZoneType::Panic;
            result[ IASZoneCluster::Alarm2 ] = IASZoneCluster::ZoneType::Emergency;
            break;

        default:
            result = Unknown;
        }
    } break;

    case IASZoneClusterAttributeId::ZoneStatus: {
        uint16_t intValue = std::stol( value );
        for( int i = 0; i < 8; ++i ) {
            int bitValue = intValue & ( 1 << i );
            switch( i ) {
            case 0:
                result[ IASZoneCluster::Alarm1 ] =
                    bitValue ? IASZoneCluster::ZoneStatus::On : IASZoneCluster::ZoneStatus::Off;
                break;

            case 1:
                result[ IASZoneCluster::Alarm2 ] =
                    bitValue ? IASZoneCluster::ZoneStatus::On : IASZoneCluster::ZoneStatus::Off;
                break;

            case 2:
                result[ IASZoneCluster::ZoneStatus::Tamper ] =
                    bitValue ? IASZoneCluster::ZoneStatus::Tampered : IASZoneCluster::ZoneStatus::NotTampered;
                break;

            case 3:
                result[ IASZoneCluster::ZoneStatus::Battery ] =
                    bitValue ? IASZoneCluster::ZoneStatus::Low : IASZoneCluster::ZoneStatus::Ok;
                break;

            case 4:
                result[ IASZoneCluster::ZoneStatus::SupervisionReports ] =
                    bitValue ? IASZoneCluster::ZoneStatus::Yes : IASZoneCluster::ZoneStatus::No;
                break;

            case 5:
                result[ IASZoneCluster::ZoneStatus::RestoreReports ] =
                    bitValue ? IASZoneCluster::ZoneStatus::Yes : IASZoneCluster::ZoneStatus::No;
                break;

            case 6:
                result[ IASZoneCluster::ZoneStatus::Trouble ] =
                    bitValue ? IASZoneCluster::ZoneStatus::Trouble : IASZoneCluster::ZoneStatus::Ok;
                break;

            case 7:
                result[ IASZoneCluster::ZoneStatus::Ac ] =
                    bitValue ? IASZoneCluster::ZoneStatus::Fault : IASZoneCluster::ZoneStatus::Ok;
                break;

            default:
                assert( !"Failed to format IASZoneClusterAttributeId::ZoneStatus" );
            }
        }
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatElectricalMeasurementAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< ElectricalMeasurementClusterAttributeId >( id ) ) {
    case ElectricalMeasurementClusterAttributeId::RMSVoltage:
    case ElectricalMeasurementClusterAttributeId::RMSCurrent: {
        uint16_t intValue = std::stoul( value );
        if( intValue == 0xFFFF ) {
            result = Undefined;
        }
        else {
            result = intValue;
        }
    } break;

    case ElectricalMeasurementClusterAttributeId::ActivePower: {
        result = std::stoi( value );
    } break;

    case ElectricalMeasurementClusterAttributeId::ACVoltageMultiplier:
    case ElectricalMeasurementClusterAttributeId::ACVoltageDivisor:
    case ElectricalMeasurementClusterAttributeId::ACCurrentMultiplier:
    case ElectricalMeasurementClusterAttributeId::ACCurrentDivisor:
    case ElectricalMeasurementClusterAttributeId::ACPowerMultiplier:
    case ElectricalMeasurementClusterAttributeId::ACPowerDivisor: {
        result = std::stoul( value );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatTemperatureMeasurementAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< TemperatureMeasurementClusterAttributeId >( id ) ) {
    case TemperatureMeasurementClusterAttributeId::MeasuredValue:
    case TemperatureMeasurementClusterAttributeId::MinMeasuredValue:
    case TemperatureMeasurementClusterAttributeId::MaxMeasuredValue: {
        int16_t intValue = std::stoi( value );
        if( intValue == static_cast< int16_t >( 0x8000 ) ) {
            result = Undefined;
        }
        else {
            result = intValue;
        }
    } break;

    case TemperatureMeasurementClusterAttributeId::Tolerance: {
        result = std::stoul( value );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatIlluminanceMeasurementAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< IlluminanceMeasurementClusterAttributeId >( id ) ) {
    case IlluminanceMeasurementClusterAttributeId::MeasuredValue:
    case IlluminanceMeasurementClusterAttributeId::MinMeasuredValue:
    case IlluminanceMeasurementClusterAttributeId::MaxMeasuredValue: {
        uint16_t intValue = std::stoul( value );
        if( intValue == 0xffff ) {
            result = Undefined;
        }
        else {
            result = intValue;
        }
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatWaterContentAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< WaterContentClusterAttributeId >( id ) ) {
    case WaterContentClusterAttributeId::MeasuredValue:
    case WaterContentClusterAttributeId::MinMeasuredValue:
    case WaterContentClusterAttributeId::MaxMeasuredValue: {
        uint16_t intValue = std::stoul( value );
        if( intValue == 0xffff ) {
            result = Undefined;
        }
        else {
            result = intValue;
        }
    } break;

    case WaterContentClusterAttributeId::Tolerance: {
        result = std::stoul( value );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatThermostatAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< ThermostatClusterAttributeId >( id ) ) {
    case ThermostatClusterAttributeId::LocalTemperature:
    case ThermostatClusterAttributeId::OccupiedCoolingSetpoint:
    case ThermostatClusterAttributeId::OccupiedHeatingSetpoint:
    case ThermostatClusterAttributeId::MinHeatSetpointLimit:
    case ThermostatClusterAttributeId::MaxHeatSetpointLimit:
    case ThermostatClusterAttributeId::MinCoolSetpointLimit:
    case ThermostatClusterAttributeId::MaxCoolSetpointLimit: {
        int16_t intValue = std::stoi( value );
        if( intValue == static_cast< int16_t >( 0x8000 ) ) {
            result = Undefined;
        }
        else {
            result = intValue;
        }
    } break;

    case ThermostatClusterAttributeId::MinSetpointDeadBand:
    case ThermostatClusterAttributeId::LocalTemperatureCalibration: {
        int8_t intValue = std::stoi( value );
        result = intValue;
    } break;

    case ThermostatClusterAttributeId::SystemMode: {
        auto enumValue = static_cast< ThermostatSystemMode >( std::stoul( value ) );
        result = toString( enumValue );
    } break;

    case ThermostatClusterAttributeId::ThermostatRunningState: {
        uint16_t intValue = std::stoul( value );
        JsonArray arr;
        for( int i = 0; i < 7; ++i ) {
            int bitValue = intValue & ( 1 << i );
            if( !bitValue ) {
                continue;
            }

            switch( i ) {
            case 0:
                arr.push_back( ThermostatCluster::Heat );
                break;

            case 1:
                arr.push_back( ThermostatCluster::Cool );
                break;

            case 2:
                arr.push_back( ThermostatCluster::ThermostatRunningState::Fan );
                break;

            case 3:
                arr.push_back( ThermostatCluster::ThermostatRunningState::Heat2ndStage );
                break;

            case 4:
                arr.push_back( ThermostatCluster::ThermostatRunningState::Cool2ndStage );
                break;

            case 5:
                arr.push_back( ThermostatCluster::ThermostatRunningState::Fan2ndStage );
                break;

            case 6:
                arr.push_back( ThermostatCluster::ThermostatRunningState::Fan3rdStage );
                break;
            }
        }

        result = std::move( arr );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatFanControlAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< FanControlClusterAttributeId >( id ) ) {
    case FanControlClusterAttributeId::FanMode: {
        auto enumValue = static_cast< ThermostatFanMode >( std::stoul( value ) );
        result = toString( enumValue );
    } break;

    case FanControlClusterAttributeId::FanModeSequence: {
        JsonArray arr;

        uint8_t intValue = std::stoul( value );
        switch( intValue ) {
        case 0x00:
            arr.push_back( toString( ThermostatFanMode::Low ) );
            arr.push_back( toString( ThermostatFanMode::Medium ) );
            arr.push_back( toString( ThermostatFanMode::High ) );
            break;

        case 0x01:
            arr.push_back( toString( ThermostatFanMode::Low ) );
            arr.push_back( toString( ThermostatFanMode::High ) );
            break;

        case 0x02:
            arr.push_back( toString( ThermostatFanMode::Low ) );
            arr.push_back( toString( ThermostatFanMode::Medium ) );
            arr.push_back( toString( ThermostatFanMode::High ) );
            arr.push_back( toString( ThermostatFanMode::Auto ) );
            break;

        case 0x03:
            arr.push_back( toString( ThermostatFanMode::Low ) );
            arr.push_back( toString( ThermostatFanMode::High ) );
            arr.push_back( toString( ThermostatFanMode::Auto ) );
            break;

        case 0x04:
            arr.push_back( toString( ThermostatFanMode::On ) );
            arr.push_back( toString( ThermostatFanMode::Auto ) );
            break;
        }

        result = std::move( arr );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatPowerConfigurationAttribute( uint16_t id, const std::string& value ) {
    using namespace Resources::Fields::Attribute::Formats;

    JsonObject result;
    switch( static_cast< PowerConfigurationClusterAttributeId >( id ) ) {
    case PowerConfigurationClusterAttributeId::BatteryVoltage:
    case PowerConfigurationClusterAttributeId::BatteryPercentageRemaining: {
        uint8_t intValue = std::stoul( value );
        if( intValue == 0xff ) {
            result = Undefined;
        }
        else {
            result = intValue;
        }
    } break;

    case PowerConfigurationClusterAttributeId::BatteryQuantity:
        result = std::stoul( value );
        break;

    case PowerConfigurationClusterAttributeId::BatterySize: {
        auto enumValue = static_cast< PowerConfBatterySize >( std::stoul( value ) );
        result = toString( enumValue );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatLevelControlAttribute( uint16_t id, const std::string& value ) {
    JsonObject result;
    switch( static_cast< LevelControlClusterAttributeId >( id ) ) {
    case LevelControlClusterAttributeId::CurrentLevel:
        result = std::stoul( value );
        break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatColorControlAttribute( uint16_t id, const std::string& value ) {
    JsonObject result;
    switch( static_cast< ColorControlClusterAttributeId >( id ) ) {
    case ColorControlClusterAttributeId::CurrentX:
    case ColorControlClusterAttributeId::CurrentY:
    case ColorControlClusterAttributeId::ColorTempPhysicalMinMireds:
    case ColorControlClusterAttributeId::ColorTempPhysicalMaxMireds:
    case ColorControlClusterAttributeId::ColorTemperatureMireds:
        result = std::stoul( value );
        break;

    case ColorControlClusterAttributeId::ColorMode: {
        auto enumValue = static_cast< ColorMode >( std::stoul( value ) );
        result = toString( enumValue );
    } break;

    case ColorControlClusterAttributeId::ColorCapabilities: {
        uint16_t bitmap = std::stoul( value );

        JsonArray capabilities;
        for( int i = 0; i < 5; ++i ) {
            if( bitmap & ( 1 << i ) ) {
                capabilities.push_back( toString( static_cast< ColorCapability >( i ) ) );
            }
        }

        result = std::move( capabilities );
    } break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatDoorLockAttribute( uint16_t id, const std::string& value ) {
    JsonObject result;
    switch( static_cast< DoorLockClusterAttributeId >( id ) ) {
    case DoorLockClusterAttributeId::LockState:
        result = toString( static_cast< LockState >( std::stoul( value ) ) );
        break;

    case DoorLockClusterAttributeId::LockType:
        result = toString( static_cast< LockType >( std::stoul( value ) ) );
        break;

    case DoorLockClusterAttributeId::ActuatorEnabled:
        result = toString( value == "true" ? DoorLockActuatorState::Enabled : DoorLockActuatorState::Disabled );
        break;

    case DoorLockClusterAttributeId::DoorState:
        result = toString( static_cast< DoorState >( std::stoul( value ) ) );
        break;

    case DoorLockClusterAttributeId::DoorOpenEvents:
    case DoorLockClusterAttributeId::DoorClosedEvents:
        result = static_cast< uint32_t >( stoul( value ) );
        break;

    case DoorLockClusterAttributeId::OpenPeriod:
    case DoorLockClusterAttributeId::NumberOfTotalUsersSupported:
    case DoorLockClusterAttributeId::NumberOfPINUsersSupported:
    case DoorLockClusterAttributeId::NumberOfRFIDUsersSupported:
        result = static_cast< uint16_t >( stoul( value ) );
        break;

    case DoorLockClusterAttributeId::MaxPINCodeLength:
    case DoorLockClusterAttributeId::MinPINCodeLength:
    case DoorLockClusterAttributeId::MaxRFIDCodeLength:
    case DoorLockClusterAttributeId::MinRFIDCodeLength:
        result = static_cast< uint8_t >( stoul( value ) );
        break;

    default:
        result = value;
    }

    return result;
}

Json AttributesFormatter::formatOccupancySensingAttribute( uint16_t id, const std::string& value ) {
    JsonObject result;
    switch( static_cast< OccupancySensingClusterAttributeId >( id ) ) {
    case OccupancySensingClusterAttributeId::Occupancy:
        result = toString( static_cast< Occupancy >( std::stoul( value ) ) );
        break;

    case OccupancySensingClusterAttributeId::OccupancySensorType:
        result = toString( static_cast< OccupancySensorType >( std::stoul( value ) ) );
        break;

    case OccupancySensingClusterAttributeId::OccupancySensorTypeBitmap: {
        JsonArray arr;

        uint8_t intValue = std::stoul( value );
        if( intValue & 0x01 ) {
            arr.push_back( toString( OccupancySensorType::Pir ) );
        }
        if( intValue & 0x02 ) {
            arr.push_back( toString( OccupancySensorType::Ultrasonic ) );
        }
        if( intValue & 0x04 ) {
            arr.push_back( toString( OccupancySensorType::PhysicalContact ) );
        }

        result = std::move( arr );
    } break;

    case OccupancySensingClusterAttributeId::PIROccupiedToUnoccupiedDelay:
    case OccupancySensingClusterAttributeId::PIRUnoccupiedToOccupiedDelay:
    case OccupancySensingClusterAttributeId::UltrasonicOccupiedToUnoccupiedDelay:
    case OccupancySensingClusterAttributeId::UltrasonicUnoccupiedToOccupiedDelay:
    case OccupancySensingClusterAttributeId::PhysicalContactOccupiedToUnoccupiedDelay:
    case OccupancySensingClusterAttributeId::PhysicalContactUnoccupiedToOccupiedDelay:
        result = static_cast< uint16_t >( stoul( value ) );
        break;

    case OccupancySensingClusterAttributeId::PIRUnoccupiedToOccupiedThreshold:
    case OccupancySensingClusterAttributeId::UltrasonicUnoccupiedToOccupiedThreshold:
    case OccupancySensingClusterAttributeId::PhysicalContactUnoccupiedToOccupiedThreshold:
        result = static_cast< uint8_t >( stoul( value ) );
        break;

    default:
        result = value;
    }

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
