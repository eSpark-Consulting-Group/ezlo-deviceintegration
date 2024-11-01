#include "zigbee/resources/zigbee_resources.h"

#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Zigbee {
namespace Resources {

namespace DB {

const std::vector< std::string > BackupTableNames = {
    TableNames::Devices,
    ZigbeeFlasherDBName,
};

namespace TableNames {

const char* const Devices = "db.addons.zigbee.devices";
const char* const DevicesReachableInited = "temp.db.addons.zigbee.devices_reachable_inited";

}  // namespace TableNames

namespace Fields {

const char* const Id = db::ID_FIELD_NAME.c_str();

}  // namespace Fields
}  // namespace DB

namespace Fields {

namespace Node {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const ModelId = "model_id";
const char* const ManufacturerId = "manufacturer_id";
const char* const Security = "security";
const char* const Endpoints = "endpoints";
const char* const IsReachable = "is_reachable";
const char* const IsBatteryPowered = "is_battery_powered";
const char* const IsSleepy = "is_sleepy";
const char* const Capabilities = "capabilities";
const char* const LastUpdateTime = "last_update_time";
const char* const ReachableStateRecheckTimeout = "reachable_state_timeout";

namespace Values {

const char* const Unknown = "Unknown";

}  // namespace Values

}  // namespace Node

namespace Endpoint {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const ProfileId = "profile_id";
const char* const DeviceId = "device_id";
const char* const Clusters = "clusters";

}  // namespace Endpoint

namespace Cluster {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const EndpointId = "endpoint_id";
const char* const Name = "name";
const char* const DataHolder = "data_holder";
const char* const Attributes = "attributes";
const char* const Commands = "commands";

namespace DataHolders {

const char* const Undefined = "undefined";
const char* const Client = "client";
const char* const Server = "server";
const char* const ClientServer = "clientServer";

}  // namespace DataHolders

namespace Names {

const char* const Basic = "basic";
const char* const PowerConfiguration = "power_configuration";
const char* const DeviceTemperatureConfiguration = "device_temperature_configuration";
const char* const Groups = "groups";
const char* const Identify = "identify";
const char* const Scenes = "scenes";
const char* const OnOffSwitchConfiguration = "on_off_switch_configuration";
const char* const OnOff = "on_off";
const char* const LevelControl = "level_control";
const char* const Time = "time";
const char* const Alarms = "alarms";
const char* const RSSILocation = "rssi_location";
const char* const PollControl = "poll_control";
const char* const Diagnostics = "diagnostics";
const char* const PowerProfile = "power_profile";
const char* const MeterIdentification = "meter_identification";
const char* const ElectricalMeasurement = "electrical_measurement";
const char* const AnalogInputBasic = "analog_input_basic";
const char* const AnalogOutputBasic = "analog_output_basic";
const char* const AnalogValueBasic = "analog_value_basic";
const char* const BinaryInputBasic = "binary_input_basic";
const char* const BinaryOutputBasic = "binary_output_basic";
const char* const BinaryValueBasic = "binary_value_basic";
const char* const MultistateInputBasic = "multistate_input_basic";
const char* const MultistateOutputBasic = "multistate_output_basic";
const char* const MultistateValueBasic = "multistate_value_basic";
const char* const OtaUpgrade = "ota_upgrade";
const char* const DoorLock = "door_lock";
const char* const Thermostat = "thermostat";
const char* const FanControl = "fan_control";
const char* const ColorControl = "color_control";
const char* const IlluminanceMeasurement = "illuminance_measurement";
const char* const TemperatureMeasurement = "temperature_measurement";
const char* const RelativeHumidity = "relative_humidity";
const char* const OccupancySensing = "occupancy_sensing";
const char* const LeafWetness = "leaf_wetness";
const char* const SoilMoisture = "soil_moisture";
const char* const ApplianceEventsAndAlerts = "appliance_events_and_alerts";
// Security clusters
const char* const IASZone = "ias_zone";
const char* const IASAce = "ias_ace";
const char* const IASWd = "ias_wd";

const char* const Unknown = "unknown";

}  // namespace Names
}  // namespace Cluster

namespace Attribute {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const ClusterId = "cluster_id";
const char* const Name = "name";
const char* const Type = "type";
const char* const IsMfgSpecific = "is_mfg_specific";
const char* const MfgCode = "mfg_code";
const char* const Value = "value";
const char* const FormattedValue = "formatted_value";

namespace ValueType {

const char* const NoData = "void";
const char* const Bool = "bool";
const char* const Bitmap8Data = "bitmap8_t";
const char* const Bitmap16Data = "bitmap16_t";
const char* const Uint8Data = "uint8_t";
const char* const Uint16Data = "uint16_t";
const char* const Uint32Data = "uint32_t";
const char* const Int8Data = "int8_t";
const char* const Int16Data = "int16_t";
const char* const Enum8Data = "enum8_t";
const char* const Enum16Data = "enum16_t";
const char* const StringData = "string";
const char* const Eui64Data = "eui64_t";
const char* const UnknownData = "unknown";

}  // namespace ValueType

namespace Names {

const char* const Unknown = "unknown";

namespace Basic {

const char* const ZCLVersion = "ZCLVersion";
const char* const ApplicationVersion = "ApplicationVersion";
const char* const StackVersion = "StackVersion";
const char* const HWVersion = "HWVersion";
const char* const ManufacturerName = "ManufacturerName";
const char* const ModelIdentifier = "ModelIdentifier";
const char* const DateCode = "DateCode";
const char* const PowerSource = "PowerSource";
const char* const GenericDeviceClass = "GenericDeviceClass";
const char* const GenericDeviceType = "GenericDeviceType";
const char* const ProductCode = "ProductCode";
const char* const ProductURL = "ProductURL";
const char* const ManufacturerVersionDetails = "ManufacturerVersionDetails";
const char* const SerialNumber = "SerialNumber";
const char* const ProductLabel = "ProductLabel";
const char* const LocationDescription = "LocationDescription";
const char* const PhysicalEnvironment = "PhysicalEnvironment";
const char* const DeviceEnabled = "DeviceEnabled";
const char* const AlarmMask = "AlarmMask";
const char* const DisableLocalConfig = "DisableLocalConfig";
const char* const SWBuildID = "SWBuildID";

}  // namespace Basic

namespace PollControl {

const char* const CheckInInterval = "CheckInInterval";
const char* const LongPollInterval = "LongPollInterval";
const char* const ShortPollInterval = "ShortPollInterval";
const char* const FastPollTimeout = "FastPollTimeout";
const char* const CheckInIntervalMin = "CheckInIntervalMin";
const char* const LongPollIntervalMin = "LongPollIntervalMin";
const char* const FastPollTimeoutMax = "FastPollTimeoutMax";

}  // namespace PollControl

namespace PowerConfiguration {

const char* const MainsVoltage = "MainsVoltage";
const char* const MainsFrequency = "MainsFrequency";
const char* const MainsAlarmMask = "MainsAlarmMask";
const char* const MainsVoltageMinThreshold = "MainsVoltageMinThreshold";
const char* const MainsVoltageMaxThreshold = "MainsVoltageMaxThreshold";
const char* const MainsVoltageDwellTripPoint = "MainsVoltageDwellTripPoint";
const char* const BatteryVoltage = "BatteryVoltage";
const char* const BatteryPercentageRemaining = "BatteryPercentageRemaining";
const char* const BatteryManufacturer = "BatteryManufacturer";
const char* const BatterySize = "BatterySize";
const char* const BatteryAHrRating = "BatteryAHrRating";
const char* const BatteryQuantity = "BatteryQuantity";
const char* const BatteryRatedVoltage = "BatteryRatedVoltage";
const char* const BatteryAlarmMask = "BatteryAlarmMask";
const char* const BatteryVoltageMinThreshold = "BatteryVoltageMinThreshold";
const char* const BatteryVoltageThreshold1 = "BatteryVoltageThreshold1";
const char* const BatteryVoltageThreshold2 = "BatteryVoltageThreshold2";
const char* const BatteryVoltageThreshold3 = "BatteryVoltageThreshold3";
const char* const BatteryPercentageMinThreshold = "BatteryPercentageMinThreshold";
const char* const BatteryPercentageThreshold1 = "BatteryPercentageThreshold1";
const char* const BatteryPercentageThreshold2 = "BatteryPercentageThreshold2";
const char* const BatteryPercentageThreshold3 = "BatteryPercentageThreshold3";
const char* const BatteryAlarmState = "BatteryAlarmState";

}  // namespace PowerConfiguration

namespace IASZone {

const char* const ZoneState = "ZoneState";
const char* const ZoneType = "ZoneType";
const char* const ZoneStatus = "ZoneStatus";

}  // namespace IASZone

namespace OnOff {

const char* const OnOff = "OnOff";
const char* const GlobalSceneControl = "GlobalSceneControl";
const char* const OnTime = "OnTime";
const char* const OffWaitTime = "OffWaitTime";

}  // namespace OnOff

namespace LevelControl {

const char* const CurrentLevel = "CurrentLevel";
const char* const RemainingTime = "RemainingTime";
const char* const OnOffTransitionTime = "OnOffTransitionTime";
const char* const OnLevel = "OnLevel";
const char* const OnTransitionTime = "OnTransitionTime";
const char* const OffTransitionTime = "OffTransitionTime";
const char* const DefaultMoveRate = "DefaultMoveRate";

}  // namespace LevelControl

namespace ElectricalMeasurement {

const char* const MeasurementType = "MeasurementType";

const char* const DCVoltage = "DCVoltage";
const char* const DCVoltageMin = "DCVoltageMin";
const char* const DCVoltageMax = "DCVoltageMax";
const char* const DCCurrent = "DCCurrent";
const char* const DCCurrentMin = "DCCurrentMin";
const char* const DCCurrentMax = "DCCurrentMax";
const char* const DCPower = "DCPower";
const char* const DCPowerMin = "DCPowerMin";
const char* const DCPowerMax = "DCPowerMax";

const char* const DCVoltageMultiplier = "DCVoltageMultiplier";
const char* const DCVoltageDivisor = "DCVoltageDivisor";
const char* const DCCurrentMultiplier = "DCCurrentMultiplier";
const char* const DCCurrentDivisor = "DCCurrentDivisor";
const char* const DCPowerMultiplier = "DCPowerMultiplier";
const char* const DCPowerDivisor = "DCPowerDivisor";

const char* const ACFrequency = "ACFrequency";
const char* const ACFrequencyMin = "ACFrequencyMin";
const char* const ACFrequencyMax = "ACFrequencyMax";
const char* const NeutralCurrent = "NeutralCurrent";
const char* const TotalActivePower = "TotalActivePower";
const char* const TotalReactivePower = "TotalReactivePower";
const char* const TotalApparentPower = "TotalApparentPower";
const char* const Measured1stHarmonicCurrent = "Measured1stHarmonicCurrent";
const char* const Measured3rdHarmonicCurrent = "Measured3rdHarmonicCurrent";
const char* const Measured5thHarmonicCurrent = "Measured5thHarmonicCurrent";
const char* const Measured7thHarmonicCurrent = "Measured7thHarmonicCurrent";
const char* const Measured9thHarmonicCurrent = "Measured9thHarmonicCurrent";
const char* const Measured11thHarmonicCurrent = "Measured11thHarmonicCurrent";
const char* const MeasuredPhase1stHarmonicCurrent = "MeasuredPhase1stHarmonicCurrent";
const char* const MeasuredPhase3rdHarmonicCurrent = "MeasuredPhase3rdHarmonicCurrent";
const char* const MeasuredPhase5thHarmonicCurrent = "MeasuredPhase5thHarmonicCurrent";
const char* const MeasuredPhase7thHarmonicCurrent = "MeasuredPhase7thHarmonicCurrent";
const char* const MeasuredPhase9thHarmonicCurrent = "MeasuredPhase9thHarmonicCurrent";
const char* const MeasuredPhase11thHarmonicCurrent = "MeasuredPhase11thHarmonicCurrent";

const char* const ACFrequencyMultiplier = "ACFrequencyMultiplier";
const char* const ACFrequencyDivisor = "ACFrequencyDivisor";
const char* const PowerMultiplier = "PowerMultiplier";
const char* const PowerDivisor = "PowerDivisor";
const char* const HarmonicCurrentMultiplier = "HarmonicCurrentMultiplier";
const char* const PhaseHarmonicCurrentMultiplier = "PhaseHarmonicCurrentMultiplier";

const char* const LineCurrent = "LineCurrent";
const char* const ActiveCurrent = "ActiveCurrent";
const char* const ReactiveCurrent = "ReactiveCurrent";
const char* const RMSVoltage = "RMSVoltage";
const char* const RMSVoltageMin = "RMSVoltageMin";
const char* const RMSVoltageMax = "RMSVoltageMax";
const char* const RMSCurrent = "RMSCurrent";
const char* const RMSCurrentMin = "RMSCurrentMin";
const char* const RMSCurrentMax = "RMSCurrentMax";
const char* const ActivePower = "ActivePower";
const char* const ActivePowerMin = "ActivePowerMin";
const char* const ActivePowerMax = "ActivePowerMax";
const char* const ReactivePower = "ReactivePower";
const char* const ApparentPower = "ApparentPower";
const char* const PowerFactor = "PowerFactor";
const char* const AverageRMSVoltageMeasurementPeriod = "AverageRMSVoltageMeasurementPeriod";
const char* const AverageRMSOverVoltageCounter = "AverageRMSOverVoltageCounter";
const char* const AverageRMSUnderVoltageCounter = "AverageRMSUnderVoltageCounter";
const char* const RMSExtremeOverVoltagePeriod = "RMSExtremeOverVoltagePeriod";
const char* const RMSExtremeUnderVoltagePeriod = "RMSExtremeUnderVoltagePeriod";
const char* const RMSVoltageSagPeriod = "RMSVoltageSagPeriod";
const char* const RMSVoltageSwellPeriod = "RMSVoltageSwellPeriod";

const char* const ACVoltageMultiplier = "ACVoltageMultiplier";
const char* const ACVoltageDivisor = "ACVoltageDivisor";
const char* const ACCurrentMultiplier = "ACCurrentMultiplier";
const char* const ACCurrentDivisor = "ACCurrentDivisor";
const char* const ACPowerMultiplier = "ACPowerMultiplier";
const char* const ACPowerDivisor = "ACPowerDivisor";

const char* const DCOverloadAlarmsMask = "DCOverloadAlarmsMask";
const char* const DCVoltageOverload = "DCVoltageOverload";
const char* const DCCurrentOverload = "DCCurrentOverload";

const char* const ACAlarmsMask = "ACAlarmsMask";
const char* const ACVoltageOverload = "ACVoltageOverload";
const char* const ACCurrentOverload = "ACCurrentOverload";
const char* const ACActivePowerOverload = "ACActivePowerOverload";
const char* const ACReactivePowerOverload = "ACReactivePowerOverload";
const char* const AverageRMSOverVoltage = "AverageRMSOverVoltage";
const char* const AverageRMSUnderVoltage = "AverageRMSUnderVoltage";
const char* const RMSExtremeOverVoltage = "RMSExtremeOverVoltage";
const char* const RMSExtremeUnderVoltage = "RMSExtremeUnderVoltage";
const char* const RMSVoltageSag = "RMSVoltageSag";
const char* const RMSVoltageSwell = "RMSVoltageSwell";

const char* const LineCurrentPhB = "LineCurrentPhB";
const char* const ActiveCurrentPhB = "ActiveCurrentPhB";
const char* const ReactiveCurrentPhB = "ReactiveCurrentPhB";
const char* const RMSVoltagePhB = "RMSVoltagePhB";
const char* const RMSVoltageMinPhB = "RMSVoltageMinPhB";
const char* const RMSVoltageMaxPhB = "RMSVoltageMaxPhB";
const char* const RMSCurrentPhB = "RMSCurrentPhB";
const char* const RMSCurrentMinPhB = "RMSCurrentMinPhB";
const char* const RMSCurrentMaxPhB = "RMSCurrentMaxPhB";
const char* const ActivePowerPhB = "ActivePowerPhB";
const char* const ActivePowerMinPhB = "ActivePowerMinPhB";
const char* const ActivePowerMaxPhB = "ActivePowerMaxPhB";
const char* const ReactivePowerPhB = "ReactivePowerPhB";
const char* const ApparentPowerPhB = "ApparentPowerPhB";
const char* const PowerFactorPhB = "PowerFactorPhB";
const char* const AverageRMSVoltageMeasurementPeriodPhB = "AverageRMSVoltageMeasurementPeriodPhB";
const char* const AverageRMSOverVoltageCounterPhB = "AverageRMSOverVoltageCounterPhB";
const char* const AverageRMSUnderVoltageCounterPhB = "AverageRMSUnderVoltageCounterPhB";
const char* const RMSExtremeOverVoltagePeriodPhB = "RMSExtremeOverVoltagePeriodPhB";
const char* const RMSExtremeUnderVoltagePeriodPhB = "RMSExtremeUnderVoltagePeriodPhB";
const char* const RMSVoltageSagPeriodPhB = "RMSVoltageSagPeriodPhB";
const char* const RMSVoltageSwellPeriodPhB = "RMSVoltageSwellPeriodPhB";

const char* const LineCurrentPhC = "LineCurrentPhC";
const char* const ActiveCurrentPhC = "ActiveCurrentPhC";
const char* const ReactiveCurrentPhC = "ReactiveCurrentPhC";
const char* const RMSVoltagePhC = "RMSVoltagePhC";
const char* const RMSVoltageMinPhC = "RMSVoltageMinPhC";
const char* const RMSVoltageMaxPhC = "RMSVoltageMaxPhC";
const char* const RMSCurrentPhC = "RMSCurrentPhC";
const char* const RMSCurrentMinPhC = "RMSCurrentMinPhC";
const char* const RMSCurrentMaxPhC = "RMSCurrentMaxPhC";
const char* const ActivePowerPhC = "ActivePowerPhC";
const char* const ActivePowerMinPhC = "ActivePowerMinPhC";
const char* const ActivePowerMaxPhC = "ActivePowerMaxPhC";
const char* const ReactivePowerPhC = "ReactivePowerPhC";
const char* const ApparentPowerPhC = "ApparentPowerPhC";
const char* const PowerFactorPhC = "PowerFactorPhC";
const char* const AverageRMSVoltageMeasurementPeriodPhC = "AverageRMSVoltageMeasurementPeriodPhC";
const char* const AverageRMSOverVoltageCounterPhC = "AverageRMSOverVoltageCounterPhC";
const char* const AverageRMSUnderVoltageCounterPhC = "AverageRMSUnderVoltageCounterPhC";
const char* const RMSExtremeOverVoltagePeriodPhC = "RMSExtremeOverVoltagePeriodPhC";
const char* const RMSExtremeUnderVoltagePeriodPhC = "RMSExtremeUnderVoltagePeriodPhC";
const char* const RMSVoltageSagPeriodPhC = "RMSVoltageSagPeriodPhC";
const char* const RMSVoltageSwellPeriodPhC = "RMSVoltageSwellPeriodPhC";

}  // namespace ElectricalMeasurement

namespace TemperatureMeasurement {

const char* const MeasuredValue = "MeasuredValue";
const char* const MinMeasuredValue = "MinMeasuredValue";
const char* const MaxMeasuredValue = "MaxMeasuredValue";
const char* const Tolerance = "Tolerance";

}  // namespace TemperatureMeasurement

namespace WaterContent {

const char* const MeasuredValue = "MeasuredValue";
const char* const MinMeasuredValue = "MinMeasuredValue";
const char* const MaxMeasuredValue = "MaxMeasuredValue";
const char* const Tolerance = "Tolerance";

}  // namespace WaterContent

namespace OccupancySensing {

const char* const Occupancy = "Occupancy";
const char* const OccupancySensorType = "OccupancySensorType";
const char* const OccupancySensorTypeBitmap = "OccupancySensorTypeBitmap";

const char* const PIROccupiedToUnoccupiedDelay = "PIROccupiedToUnoccupiedDelay";
const char* const PIRUnoccupiedToOccupiedDelay = "PIRUnoccupiedToOccupiedDelay";
const char* const PIRUnoccupiedToOccupiedThreshold = "PIRUnoccupiedToOccupiedThreshold";

const char* const UltrasonicOccupiedToUnoccupiedDelay = "UltrasonicOccupiedToUnoccupiedDelay";
const char* const UltrasonicUnoccupiedToOccupiedDelay = "UltrasonicUnoccupiedToOccupiedDelay";
const char* const UltrasonicUnoccupiedToOccupiedThreshold = "UltrasonicUnoccupiedToOccupiedThreshold";

const char* const PhysicalContactOccupiedToUnoccupiedDelay = "PhysicalContactOccupiedToUnoccupiedDelay";
const char* const PhysicalContactUnoccupiedToOccupiedDelay = "PhysicalContactUnoccupiedToOccupiedDelay";
const char* const PhysicalContactUnoccupiedToOccupiedThreshold = "PhysicalContactUnoccupiedToOccupiedThreshold";

}  // namespace OccupancySensing

namespace IlluminanceMeasurement {

const char* const MeasuredValue = "MeasuredValue";
const char* const MinMeasuredValue = "MinMeasuredValue";
const char* const MaxMeasuredValue = "MaxMeasuredValue";
const char* const Tolerance = "Tolerance";
const char* const LightSensorType = "LightSensorType";

}  // namespace IlluminanceMeasurement

namespace DoorLock {

const char* const LockState = "LockState";
const char* const LockType = "LockType";
const char* const ActuatorEnabled = "ActuatorEnabled";
const char* const DoorState = "DoorState";
const char* const DoorOpenEvents = "DoorOpenEvents";
const char* const DoorClosedEvents = "DoorClosedEvents";
const char* const OpenPeriod = "OpenPeriod";

const char* const NumberOfLogRecordsSupported = "NumberOfLogRecordsSupported";
const char* const NumberOfTotalUsersSupported = "NumberOfTotalUsersSupported";
const char* const NumberOfPINUsersSupported = "NumberOfPINUsersSupported";
const char* const NumberOfRFIDUsersSupported = "NumberOfRFIDUsersSupported";
const char* const NumberOfWeekDaySchedulesSupportedPerUser = "NumberOfWeekDaySchedulesSupportedPerUser";
const char* const NumberOfYearDaySchedulesSupportedPerUser = "NumberOfYearDaySchedulesSupportedPerUser";
const char* const NumberOfHolidaySchedulesSupported = "NumberOfHolidaySchedulesSupported";
const char* const MaxPINCodeLength = "MaxPINCodeLength";
const char* const MinPINCodeLength = "MinPINCodeLength";
const char* const MaxRFIDCodeLength = "MaxRFIDCodeLength";
const char* const MinRFIDCodeLength = "MinRFIDCodeLength";

const char* const EnableLogging = "EnableLogging";
const char* const Language = "Language";
const char* const LEDSettings = "LEDSettings";
const char* const AutoRelockTime = "AutoRelockTime";
const char* const SoundVolume = "SoundVolume";
const char* const OperatingMode = "OperatingMode";
const char* const SupportedOperatingModes = "SupportedOperatingModes";
const char* const DefaultConfigurationRegister = "DefaultConfigurationRegister";
const char* const EnableLocalProgramming = "EnableLocalProgramming";
const char* const EnableOneTouchLocking = "EnableOneTouchLocking";
const char* const EnableInsideStatusLED = "EnableInsideStatusLED";
const char* const EnablePrivacyModeButton = "EnablePrivacyModeButton";

const char* const WrongCodeEntryLimit = "WrongCodeEntryLimit";
const char* const UserCodeTemporaryDisableTime = "UserCodeTemporaryDisableTime";
const char* const SendPINOverTheAir = "SendPINOverTheAir";
const char* const RequirePINforRFOperation = "RequirePINforRFOperation";
const char* const SecurityLevel = "SecurityLevel";

const char* const AlarmMask = "AlarmMask";
const char* const KeypadOperationEventMask = "KeypadOperationEventMask";
const char* const RFOperationEventMask = "RFOperationEventMask";
const char* const ManualOperationEventMask = "ManualOperationEventMask";
const char* const RFIDOperationEventMask = "RFIDOperationEventMask";
const char* const KeypadProgrammingEventMask = "KeypadProgrammingEventMask";
const char* const RFProgrammingEventMask = "RFProgrammingEventMask";
const char* const RFIDProgrammingEventMask = "RFIDProgrammingEventMask";

}  // namespace DoorLock

namespace Thermostat {

const char* const LocalTemperature = "LocalTemperature";
const char* const OutdoorTemperature = "OutdoorTemperature";
const char* const Occupancy = "Occupancy";
const char* const AbsMinHeatSetpointLimit = "AbsMinHeatSetpointLimit";
const char* const AbsMaxHeatSetpointLimit = "AbsMaxHeatSetpointLimit";
const char* const AbsMinCoolSetpointLimit = "AbsMinCoolSetpointLimit";
const char* const AbsMaxCoolSetpointLimit = "AbsMaxCoolSetpointLimit";
const char* const PICoolingDemand = "PICoolingDemand";
const char* const PIHeatingDemand = "PIHeatingDemand";
const char* const HVACSystemTypeConfiguration = "HVACSystemTypeConfiguration";

const char* const LocalTemperatureCalibration = "LocalTemperatureCalibration";
const char* const OccupiedCoolingSetpoint = "OccupiedCoolingSetpoint";
const char* const OccupiedHeatingSetpoint = "OccupiedHeatingSetpoint";
const char* const UnoccupiedCoolingSetpoint = "UnoccupiedCoolingSetpoint";
const char* const UnoccupiedHeatingSetpoint = "UnoccupiedHeatingSetpoint";
const char* const MinHeatSetpointLimit = "MinHeatSetpointLimit";
const char* const MaxHeatSetpointLimit = "MaxHeatSetpointLimit";
const char* const MinCoolSetpointLimit = "MinCoolSetpointLimit";
const char* const MaxCoolSetpointLimit = "MaxCoolSetpointLimit";
const char* const MinSetpointDeadBand = "MinSetpointDeadBand";
const char* const RemoteSensing = "RemoteSensing";
const char* const ControlSequenceOfOperation = "ControlSequenceOfOperation";
const char* const SystemMode = "SystemMode";
const char* const AlarmMask = "AlarmMask";
const char* const ThermostatRunningMode = "ThermostatRunningMode";
const char* const ThermostatRunningState = "ThermostatRunningState";

}  // namespace Thermostat

namespace FanControl {

const char* const FanMode = "FanMode";
const char* const FanModeSequence = "FanModeSequence";

}  // namespace FanControl

namespace ColorControl {

const char* const CurrentHue = "CurrentHue";
const char* const CurrentSaturation = "CurrentSaturation";
const char* const RemainingTime = "RemainingTime";
const char* const CurrentX = "CurrentX";
const char* const CurrentY = "CurrentY";
const char* const DriftCompensation = "DriftCompensation";
const char* const CompensationText = "CompensationText";
const char* const ColorTemperatureMireds = "ColorTemperatureMireds";
const char* const ColorMode = "ColorMode";
const char* const Options = "Options";
const char* const EnhancedCurrentHue = "EnhancedCurrentHue";
const char* const EnhancedColorMode = "EnhancedColorMode";
const char* const ColorLoopActive = "ColorLoopActive";
const char* const ColorLoopDirection = "ColorLoopDirection";
const char* const ColorLoopTime = "ColorLoopTime";
const char* const ColorLoopStartEnhancedHue = "ColorLoopStartEnhancedHue";
const char* const ColorLoopStoredEnhancedHue = "ColorLoopStoredEnhancedHue";
const char* const ColorCapabilities = "ColorCapabilities";
const char* const ColorTempPhysicalMinMireds = "ColorTempPhysicalMinMireds";
const char* const ColorTempPhysicalMaxMireds = "ColorTempPhysicalMaxMireds";
const char* const CoupleColorTempToLevelMinMireds = "CoupleColorTempToLevelMinMireds";
const char* const StartUpColorTemperatureMireds = "StartUpColorTemperatureMireds";

const char* const NumberOfPrimaries = "NumberOfPrimaries";
const char* const Primary1X = "Primary1X";
const char* const Primary1Y = "Primary1Y";
const char* const Primary1Intensity = "Primary1Intensity";
const char* const Primary2X = "Primary2X";
const char* const Primary2Y = "Primary2Y";
const char* const Primary2Intensity = "Primary2Intensity";
const char* const Primary3X = "Primary3X";
const char* const Primary3Y = "Primary3Y";
const char* const Primary3Intensity = "Primary3Intensity";

const char* const Primary4X = "Primary4X";
const char* const Primary4Y = "Primary4Y";
const char* const Primary4Intensity = "Primary4Intensity";
const char* const Primary5X = "Primary5X";
const char* const Primary5Y = "Primary5Y";
const char* const Primary5Intensity = "Primary5Intensity";
const char* const Primary6X = "Primary6X";
const char* const Primary6Y = "Primary6Y";
const char* const Primary6Intensity = "Primary6Intensity";

const char* const WhitePointX = "WhitePointX";
const char* const WhitePointY = "WhitePointY";
const char* const ColorPointRX = "ColorPointRX";
const char* const ColorPointRY = "ColorPointRY";
const char* const ColorPointRIntensity = "ColorPointRIntensity";
const char* const ColorPointGX = "ColorPointGX";
const char* const ColorPointGY = "ColorPointGY";
const char* const ColorPointGIntensity = "ColorPointGIntensity";
const char* const ColorPointBX = "ColorPointBX";
const char* const ColorPointBY = "ColorPointBY";
const char* const ColorPointBIntensity = "ColorPointBIntensity";

}  // namespace ColorControl

namespace MultistateInputBasic {

const char* const StateText = "StateText";
const char* const Description = "Description";
const char* const NumberOfStates = "NumberOfStates";
const char* const OutOfService = "OutOfService";
const char* const PresentValue = "PresentValue";
const char* const Reliability = "Reliability";
const char* const StatusFlags = "StatusFlags";
const char* const ApplicationType = "ApplicationType";

}  // namespace MultistateInputBasic
}  // namespace Names

namespace Formats {

const char* const Undefined = "undefined";
const char* const Unknown = "unknown";

namespace IASZoneCluster {

const char* const Alarm1 = "alarm_1";
const char* const Alarm2 = "alarm_2";

namespace ZoneState {

const char* const NotEnrolled = "not_entrolled";
const char* const Enrolled = "enrolled";

}  // namespace ZoneState

namespace ZoneType {

const char* const ZoneType = "zone_type";

const char* const StandardCie = "standard_cie";
const char* const MotionSensor = "motion_sensor";
const char* const ContactSwitch = "contact_switch";
const char* const FireSensor = "fire_sensor";
const char* const WaterSensor = "water_sensor";
const char* const GasSensor = "gas_sensor";
const char* const PersonalEmergencyDevice = "personal_emergency_device";
const char* const VibrationMovementSensor = "vibration_movement_sensor";
const char* const RemoteControl = "remote_control";
const char* const KeyFob = "key_fob";
const char* const Keypad = "keypad";

const char* const SystemAlarm = "system_alarm";
const char* const IntrusionIndication = "intrusion_indication";
const char* const PresenceIndication = "presence_indication";
const char* const FirstPortal = "first_portal";
const char* const SecondPortal = "second_portal";
const char* const FireIndication = "fire_indication";
const char* const WaterOverflow = "water_overflow";
const char* const CoIndication = "co_indication";
const char* const CoocingIndication = "coocing_indication";
const char* const Fall = "fall";
const char* const Button = "button";
const char* const MovementIndication = "movement_indication";
const char* const Vibration = "vibration";
const char* const Panic = "panic";
const char* const Emergency = "emergency";

}  // namespace ZoneType

namespace ZoneStatus {

const char* const Tamper = "tamper";
const char* const Battery = "battery";
const char* const SupervisionReports = "supervision_reports";
const char* const RestoreReports = "restore_reports";
const char* const Trouble = "trouble";
const char* const Ac = "ac";

const char* const On = "on";
const char* const Off = "off";
const char* const Tampered = "tampered";
const char* const NotTampered = "not_tampered";
const char* const Low = "low";
const char* const Ok = "ok";
const char* const Yes = "yes";
const char* const No = "no";
const char* const Fault = "fault";

}  // namespace ZoneStatus
}  // namespace IASZoneCluster

namespace ThermostatCluster {

const char* const Off = "off";
const char* const Cool = "cool";
const char* const Heat = "heat";

namespace SystemMode {

const char* const Auto = "auto";
const char* const EmergencyHeating = "emergency_heating";
const char* const Precooling = "precooling";
const char* const FanOnly = "fan_only";
const char* const Dry = "dry";
const char* const Sleep = "sleep";

}  // namespace SystemMode

namespace ThermostatRunningState {

const char* const Fan = "fan";
const char* const Heat2ndStage = "heat_2nd_stage";
const char* const Cool2ndStage = "cool_2nd_stage";
const char* const Fan2ndStage = "fan_2nd_stage";
const char* const Fan3rdStage = "fan_3rd_stage";

}  // namespace ThermostatRunningState
}  // namespace ThermostatCluster

namespace FanControlCluster {

const char* const Off = "off";
const char* const Low = "low";
const char* const Medium = "medium";
const char* const High = "high";
const char* const On = "on";
const char* const Auto = "auto";

}  // namespace FanControlCluster

namespace PowerConfigurationCluster {
namespace BatterySize {

const char* const NoBattery = "no_battery";
const char* const BuiltIn = "built_in";
const char* const Other = "other";
const char* const AA = "AA";
const char* const AAA = "AAA";
const char* const C = "C";
const char* const D = "D";
const char* const CR2 = "CR2";
const char* const CR123A = "CR123A";

}  // namespace BatterySize
}  // namespace PowerConfigurationCluster

namespace ColorControlCluster {
namespace ColorMode {

const char* const ColorHueSaturation = "color_hue_saturation";
const char* const ColorXY = "color_xy";
const char* const ColorTemperature = "color_temperature";

}  // namespace ColorMode

namespace ColorCapabilities {

const char* const HueSaturationSupported = "hue_saturation_supported";
const char* const EnhancedHueSupported = "enhanced_hue_supported";
const char* const ColorLoopSupported = "color_loop_supported";
const char* const XyAttributesSupported = "xy_attributes_supported";
const char* const ColorTemperatureSupported = "color_temperature_supported";

}  // namespace ColorCapabilities
}  // namespace ColorControlCluster

namespace DoorLockCluster {
namespace LockState {

const char* const NotFullyLocked = "not_fully_locked";
const char* const Locked = "locked";
const char* const Unlocked = "unlocked";

}  // namespace LockState

namespace LockType {

const char* const DeadBolt = "dead_bolt";
const char* const Magnetic = "magnetic";
const char* const Other = "other";
const char* const Mortise = "mortise";
const char* const Rim = "rim";
const char* const LatchBolt = "latch_bolt";
const char* const CylindricalLock = "cylindrical_lock";
const char* const TubularLock = "tubular_lock";
const char* const InterconnectedLock = "interconnected_lock";
const char* const DeadLatch = "dead_latch";
const char* const DoorFurniture = "door_furniture";

}  // namespace LockType

namespace DoorLockActuatorState {

const char* const Disabled = "disabled";
const char* const Enabled = "enabled";

}  // namespace DoorLockActuatorState

namespace DoorState {

const char* const Open = "open";
const char* const Closed = "closed";
const char* const ErrorJammed = "error_jammed";
const char* const ErrorForcedOpen = "error_forced_open";
const char* const ErrorUnspecified = "error_unspecified";

}  // namespace DoorState
}  // namespace DoorLockCluster

namespace OccupancySensingCluster {
namespace Occupancy {

const char* const Unoccupied = "unoccupied";
const char* const Occupied = "occupied";

}  // namespace Occupancy

namespace OccupancySensorType {

const char* const Pir = "pir";
const char* const Ultrasonic = "ultrasonic";
const char* const PirAndUltrasonic = "pir_and_ultrasonic";
const char* const PhysicalContact = "physical_contact";

}  // namespace OccupancySensorType
}  // namespace OccupancySensingCluster
}  // namespace Formats
}  // namespace Attribute

namespace Command {

const char* const Id = db::ID_FIELD_NAME.c_str();
const char* const ClusterId = "cluster_id";
const char* const Name = "name";
const char* const Type = "type";
const char* const IsMfgSpecific = "is_mfg_specific";
const char* const MfgCode = "mfg_code";

namespace CommandType {

const char* const Generated = "generated";
const char* const Received = "received";
const char* const Unknown = "unknown";

}  // namespace CommandType

namespace NodeType {

const char* const Unknown = "unknown";
const char* const Coordinator = "coordinator";
const char* const Router = "router";
const char* const EndDevice = "end_device";
const char* const SleepyEndDevice = "sleepy_end_device";

}  // namespace NodeType

namespace Names {

const char* const Unknown = "Unknown";

namespace IasAce {

// Server
const char* const ArmResponse = "ArmResponse";
const char* const GetZoneIDMapResponse = "GetZoneIDMapResponse";
const char* const GetZoneInformationResponse = "GetZoneInformationResponse";
const char* const ZoneStatusChanged = "ZoneStatusChanged";
const char* const PanelStatusChanged = "PanelStatusChanged";
const char* const GetPanelStatusResponse = "GetPanelStatusResponse";
const char* const SetBypassedZoneList = "SetBypassedZoneList";
const char* const BypassResponse = "BypassResponse";
const char* const GetZoneStatusResponse = "GetZoneStatusResponse";

// Client
const char* const Arm = "Arm";
const char* const Bypass = "Bypass";
const char* const Emergency = "Emergency";
const char* const Fire = "Fire";
const char* const Panic = "Panic";
const char* const GetZoneIDMap = "GetZoneIDMap";
const char* const GetZoneInformation = "GetZoneInformation";
const char* const GetPanelStatus = "GetPanelStatus";
const char* const GetBypassedZoneList = "GetBypassedZoneList";
const char* const GetZoneStatus = "GetZoneStatus";

}  // namespace IasAce

namespace PollControl {

// Server
const char* const CheckIn = "CheckIn";

// Client
const char* const CheckInResponse = "CheckInResponse";
const char* const FastPollStop = "FastPollStop";
const char* const SetLongPollInterval = "SetLongPollInterval";
const char* const SetShortPollInterval = "SetShortPollInterval";

}  // namespace PollControl

namespace OnOff {
// Client
const char* const Off = "Off";
const char* const On = "On";
const char* const Toggle = "Toggle";
const char* const OffWithEffect = "OffWithEffect";
const char* const OnWithRecallGlobalScene = "OnWithRecallGlobalScene";
const char* const OnWithTimedOff = "OnWithTimedOff";

}  // namespace OnOff

namespace LevelControl {
// Client
const char* const MoveToLevel = "MoveToLevel";
const char* const Move = "Move";
const char* const Step = "Step";
const char* const Stop = "Stop";
const char* const MoveToLevelWithOnOff = "MoveToLevelWithOnOff";
const char* const MoveWithOnOff = "MoveWithOnOff";
const char* const StepWithOnOff = "StepWithOnOff";
const char* const StopWithOnOff = "StopWithOnOff";
const char* const MoveToClosestFrequency = "MoveToClosestFrequency";

}  // namespace LevelControl

namespace ColorControl {
// Client
const char* const MoveToHue = "MoveToHue";
const char* const MoveHue = "MoveHue";
const char* const StepHue = "StepHue";
const char* const MoveToSaturation = "MoveToSaturation";
const char* const MoveSaturation = "MoveSaturation";
const char* const StepSaturation = "StepSaturation";
const char* const MoveToHueAndSaturation = "MoveToHueAndSaturation";
const char* const MoveToColor = "MoveToColor";
const char* const MoveColor = "MoveColor";
const char* const StepColor = "StepColor";
const char* const MoveToColorTemperature = "MoveToColorTemperature";
const char* const EnhancedMoveToHue = "EnhancedMoveToHue";
const char* const EnhancedMoveHue = "EnhancedMoveHue";
const char* const EnhancedStepHue = "EnhancedStepHue";
const char* const EnhancedMoveToHueAndSaturation = "EnhancedMoveToHueAndSaturation";
const char* const ColorLoopSet = "ColorLoopSet";
const char* const StopMoveStep = "StopMoveStep";
const char* const MoveColorTemperature = "MoveColorTemperature";
const char* const StepColorTemperature = "StepColorTemperature";

}  // namespace ColorControl

namespace ApplianceEventsAndAlerts {
const char* const AlertsNotification = "AlertsNotification";
const char* const GetAlertsResponse = "GetAlertsResponse";
}  // namespace ApplianceEventsAndAlerts

namespace DoorLock {
// Server
const char* const LockDoorResponse = "LockDoorResponse";
const char* const UnlockDoorResponse = "UnlockDoorResponse";
const char* const ToggleResponse = "ToggleResponse";
const char* const UnlockWithTimeoutResponse = "UnlockWithTimeoutResponse";
const char* const GetLogRecordResponse = "GetLogRecordResponse";
const char* const SetPinCodeResponse = "SetPinCodeResponse";
const char* const GetPinCodeResponse = "GetPinCodeResponse";
const char* const ClearPinCodeResponse = "ClearPinCodeResponse";
const char* const ClearAllPinCodesResponse = "ClearAllPinCodesResponse";
const char* const SetUserStatusResponse = "SetUserStatusResponse";
const char* const GetUserStatusResponse = "GetUserStatusResponse";
const char* const SetWeekdayScheduleResponse = "SetWeekdayScheduleResponse";
const char* const GetWeekdayScheduleResponse = "GetWeekdayScheduleResponse";
const char* const ClearWeekdayScheduleResponse = "ClearWeekdayScheduleResponse";
const char* const SetYearDayScheduleResponse = "SetYearDayScheduleResponse";
const char* const GetYearDayScheduleResponse = "GetYearDayScheduleResponse";
const char* const ClearYearDayScheduleResponse = "ClearYearDayScheduleResponse";
const char* const SetHolidayScheduleResponse = "SetHolidayScheduleResponse";
const char* const GetHolidayScheduleResponse = "GetHolidayScheduleResponse";
const char* const ClearHolidayScheduleResponse = "ClearHolidayScheduleResponse";
const char* const SetUserTypeResponse = "SetUserTypeResponse";
const char* const GetUserTypeResponse = "GetUserTypeResponse";
const char* const SetRfidCodeResponse = "SetRfidCodeResponse";
const char* const GetRfidCodeResponse = "GetRfidCodeResponse";
const char* const ClearRfidCodeResponse = "ClearRfidCodeResponse";
const char* const ClearAllRfidCodesResponse = "ClearAllRfidCodesResponse";
const char* const OperatingEventNotification = "OperatingEventNotification";
const char* const ProgrammingEventNotification = "ProgrammingEventNotification";

// Client
const char* const LockDoor = "LockDoor";
const char* const UnlockDoor = "UnlockDoor";
const char* const Toggle = "Toggle";
const char* const UnlockWithTimeout = "UnlockWithTimeout";
const char* const GetLogRecord = "GetLogRecord";
const char* const SetPinCode = "SetPinCode";
const char* const GetPinCode = "GetPinCode";
const char* const ClearPinCode = "ClearPinCode";
const char* const ClearAllPinCodes = "ClearAllPinCodes";
const char* const SetUserStatus = "SetUserStatus";
const char* const GetUserStatus = "GetUserStatus";
const char* const SetWeekdaySchedule = "SetWeekdaySchedule";
const char* const GetWeekdaySchedule = "GetWeekdaySchedule";
const char* const ClearWeekdaySchedule = "ClearWeekdaySchedule";
const char* const SetYearDaySchedule = "SetYearDaySchedule";
const char* const GetYearDaySchedule = "GetYearDaySchedule";
const char* const ClearYearDaySchedule = "ClearYearDaySchedule";
const char* const SetHolidaySchedule = "SetHolidaySchedule";
const char* const GetHolidaySchedule = "GetHolidaySchedule";
const char* const ClearHolidaySchedule = "ClearHolidaySchedule";
const char* const SetUserType = "SetUserType";
const char* const GetUserType = "GetUserType";
const char* const SetRfidCode = "SetRfidCode";
const char* const GetRfidCode = "GetRfidCode";
const char* const ClearRfidCode = "ClearRfidCode";
const char* const ClearAllRfidCodes = "ClearAllRfidCodes";

}  // namespace DoorLock
}  // namespace Names

namespace Formats {

namespace AlertStructure {
const char* const Reserved = "reserved";
namespace Category {
const char* const Warning = "warning";
const char* const Danger = "danger";
const char* const Failure = "failure";
}  // namespace Category
namespace PresenceRecovery {
const char* const Recovery = "recovery";
const char* const Presence = "presence";
}  // namespace PresenceRecovery
}  // namespace AlertStructure

const char* const Unknown = "unknown";

namespace IasAce {
namespace ArmMode {

const char* const Disarm = "disarm";
const char* const ArmDayHomeZonesOnly = "arm_day_home_zones_only";
const char* const ArmNightSleepZonesOnly = "arm_night_home_zones_only";
const char* const ArmAllZones = "arm_all_zones";

}  // namespace ArmMode

namespace ArmResponse {

const char* const AllZonesDisarmed = "all_zones_disarmed";
const char* const OnlyDayHomeZonesArmed = "only_day_home_zones_armed";
const char* const OnlyNightSleepZonesArmed = "only_night_sleep_zones_armed";
const char* const AllZonesArmed = "all_zones_armed";
const char* const InvalidArmDisarmCode = "invalid_arm_disarm_code";
const char* const NotReadyToArm = "not_ready_to_arm";
const char* const AlreadyDisarmed = "already_disarmed";

}  // namespace ArmResponse

namespace PanelStatus {

const char* const PanelDisarmed = "panel_disarmed";
const char* const ArmedStay = "armed_stay";
const char* const ArmedNight = "armed_night";
const char* const ArmedAway = "armed_away";
const char* const ExitDelay = "exit_delay";
const char* const EntryDelay = "entry_delay";
const char* const NotReadyToArm = "not_ready_to_arm";
const char* const InAlarm = "in_alarm";
const char* const ArmingStay = "arming_stay";
const char* const ArmingNight = "arming_night";
const char* const ArmingAway = "arming_away";

}  // namespace PanelStatus

namespace AlarmStatus {

const char* const NoAlarm = "no_alarm";
const char* const Burglar = "burglar";
const char* const Fire = "fire";
const char* const Emergency = "emergency";
const char* const PolicePanic = "police_panic";
const char* const FirePanic = "fire_panic";
const char* const EmergencyPanic = "emergency_panic";

}  // namespace AlarmStatus
}  // namespace IasAce

namespace DoorLock {

const char* const NotSupported = "not_supported";

namespace UserStatus {

const char* const Available = "available";
const char* const OccupiedEnabled = "occupied_enabled";
const char* const OccupiedDisabled = "occupied_disabled";

}  // namespace UserStatus

namespace UserType {

const char* const UnrestrictedUser = "unrestricted_user";
const char* const YearDayScheduleUser = "year_day_schedule_user";
const char* const WeekDayScheduleUser = "week_day_schedule_user";
const char* const MasterUser = "master_user";
const char* const NonAccessUser = "non_access_user";

}  // namespace UserType

namespace EventSource {

const char* const Keypad = "keypad";
const char* const RF = "rf";
const char* const ManualOrReserved = "manual_or_reserved";
const char* const RFID = "rfid";
const char* const Indeterminate = "indeterminate";

}  // namespace EventSource

namespace OperationEvent {

const char* const UnknownOrMfgSpecific = "unknown_or_mfg_specific";
const char* const Lock = "lock";
const char* const Unlock = "unlock";
const char* const LockFailureInvalidPINorID = "lock_failure_invalid_pin_or_id";
const char* const LockFailureInvalidSchedule = "lock_failure_invalid_schedule";
const char* const UnlockFailureInvalidPINorID = "unlock_failure_invalid_pin_or_id";
const char* const UnlockFailureInvalidSchedule = "unlock_failure_invalid_schedule";
const char* const OneTouchLock = "one_touch_lock";
const char* const KeyLock = "key_lock";
const char* const KeyUnlock = "key_unlock";
const char* const AutoLock = "auto_lock";
const char* const ScheduleLock = "schedule_lock";
const char* const ScheduleUnlock = "schedule_unlock";
const char* const ManualLock = "manual_lock";
const char* const ManualUnlock = "manual_unlock";
const char* const NonAccessUserOperationalEvent = "non_access_user_operational_event";

}  // namespace OperationEvent

namespace ProgrammingEvent {

const char* const UnknownOrMfgSpecific = "unknown_or_mfg_specific";
const char* const MasterCodeChanged = "master_code_changed";
const char* const PINCodeAdded = "pin_code_added";
const char* const PINCodeDeleted = "pin_code_deleted";
const char* const PINCodeChanged = "pin_code_changed";
const char* const RFIDCodeAdded = "rfig_code_added";
const char* const RFIDCodeDeleted = "rfid_code_deleted";

}  // namespace ProgrammingEvent
}  // namespace DoorLock
}  // namespace Formats
}  // namespace Command

namespace NetworkInfo {

const char* const NodeId = "node_id";
const char* const PanId = "pan_id";
const char* const RadioPower = "radio_power";
const char* const RadioChannel = "radio_channel";
const char* const EpanId = "epan_id";
const char* const Euid64 = "euid_64";
const char* const GlobalKey = "global_key";
const char* const GlobalFc = "global_fc";
const char* const NetworkKey = "network_key";
const char* const NetworkFc = "network_fc";
const char* const NetworkKeySeqNumber = "network_key_seq_number";

namespace ChildInfo {

namespace Type {

const char* const Unknown = "unknown";
const char* const Coordinator = "coordinator";
const char* const Router = "router";
const char* const EndDevice = "end_device";
const char* const SleepyEndDevice = "sleepy_end_device";

}  // namespace Type

namespace Timeout {

const char* const TenSec = "10 sec";
const char* const Minutes = "min";

}  // namespace Timeout
}  // namespace ChildInfo

namespace RouteInfo {

namespace ConcentratorType {

const char* const Unknown = "unknown";
const char* const Low = "low";
const char* const High = "high";

}  // namespace ConcentratorType

namespace Status {

const char* const Active = "active";
const char* const Discovering = "discovering";
const char* const Unknown = "unknown";
const char* const Unused = "unused";

}  // namespace Status
}  // namespace RouteInfo
}  // namespace NetworkInfo
}  // namespace Fields

namespace ScriptEvent {
namespace Param {

const char* const NodeId = "node_id";
const char* const EndpointId = "endpoint_id";
const char* const ClusterId = "cluster_id";
const char* const ClusterName = "cluster_name";
const char* const Attribute = "attribute";
const char* const AttributeId = "attribute_id";
const char* const AttributeName = "attribute_name";
const char* const ManufacturerId = "manufacturer_id";
const char* const ModelId = "model_id";
const char* const Status = "status";
const char* const ArmMode = "arm_mode";
const char* const PinCode = "pin_code";
const char* const UserId = "user_id";
const char* const UserStatus = "user_status";
const char* const UserType = "user_type";
const char* const EventSource = "event_source";
const char* const EventName = "event_name";
const char* const CommandId = "command_id";
const char* const CommandName = "command_name";
const char* const Value = "value";
const char* const AlertId = "alert_id";
const char* const Category = "category";
const char* const PresenceRecovery = "presence_recovery";
const char* const ManufacturerSpecificBits = "manufacturer_specific_bits";

namespace Color {

const char* const Wwhite = "wwhite";
const char* const Cwhite = "cwhite";
const char* const Red = "red";
const char* const Green = "green";
const char* const Blue = "blue";
const char* const Amber = "amber";
const char* const Cyan = "cyan";
const char* const Purple = "purple";

}  // namespace Color

namespace ActionStatus {

const char* const Invoked = "invoked";
const char* const Started = "started";
const char* const Failed = "failed";
const char* const Timeout = "timeout";
const char* const Finished = "finished";
const char* const InProgress = "in_progress";
const char* const Busy = "busy";
const char* const Empty = "empty";
const char* const Undefined = "undefined";

}  // namespace ActionStatus

namespace ChildAndRouteInfo {

const char* const ControllerNodeId = "controller_node_id";
const char* const Type = "type";
const char* const Timeout = "timeout";
const char* const Children = "children";
const char* const Lqi = "lqi";
const char* const Age = "age";
const char* const InCost = "in_cost";
const char* const OutCost = "out_cost";
const char* const Neighbors = "neighbors";
const char* const To = "to";
const char* const Through = "through";
const char* const ConcentratorType = "concentrator_type";
const char* const Routes = "routes";

}  // namespace ChildAndRouteInfo
}  // namespace Param

namespace Name {

const char* const NodeAdded = "node_added";
const char* const NodeRejoined = "node_rejoined";
const char* const MfgInterviewFinished = "mfg_interview_finished";
const char* const NodeRemoved = "node_removed";
const char* const AttributeValueUpdated = "attribute_value_updated";
const char* const UnknownAttributeValueUpdated = "unknown_attribute_value_updated";
const char* const MfgSpecificAttributeValueUpdated = "mfg_specific_attribute_value_updated";
const char* const IncludeDevice = "include_device";
const char* const IncludeDeviceProgress = "include_device_progress";
const char* const UserCodesRequestProgress = "user_codes_request_progress";
const char* const ModuleReset = "module_reset";
const char* const SimpleCommandReceived = "simple_command_received";
const char* const ArmCommandReceived = "arm_command_received";
const char* const GetUserResponseCommandReceived = "get_user_response_command_received";
const char* const DoorLockOperatingEventNotificationReceived = "door_lock_operating_event_notification_received";
const char* const DoorLockProgrammingEventNotificationReceived = "door_lock_programming_event_notification_received";
const char* const ReachableStateUpdated = "reachable_state_updated";
const char* const AlertReceived = "alert_received";
const char* const EmptyAlertsListReceived = "empty_alerts_list_received";
const char* const NetworkInfoUpdated = "network_info_updated";
const char* const ChildAndRouteInfoCollected = "child_and_route_info_collected";

}  // namespace Name
}  // namespace ScriptEvent

namespace RPCParams {

const char* const NodeId = "nodeId";
const char* const EndpointId = "endpointId";
const char* const ClusterId = "clusterId";
const char* const ClusterName = "clusterName";
const char* const AttributeName = "attributeName";
const char* const AttributeId = "attributeId";
const char* const MinInterval = "minInterval";
const char* const MaxInterval = "maxInterval";
const char* const ReportableChange = "reportableChange";
const char* const CommandName = "commandName";
const char* const CommandParams = "CommandParams";
const char* const Value = "value";
const char* const MfgCode = "mfgCode";
const char* const StartUserId = "startUserId";
const char* const EndUserId = "endUserId";
const char* const InterviewConfigurationsFile = "interviewConfigurationsFile";

}  // namespace RPCParams

namespace CommandParams {
namespace LevelControlCluster {

const char* const UpParam = "up";
const char* const DownParam = "down";

}  // namespace LevelControlCluster

namespace ColorControlCluster {

const char* const CurrentX = "current_x";
const char* const CurrentY = "current_y";

}  // namespace ColorControlCluster

namespace PollControlCluster {

const char* const StartFastPollingMode = "start_fast_polling_mode";
const char* const FastPollTimeout = "fast_poll_timeout";

}  // namespace PollControlCluster

namespace DoorLockCluster {

const char* const UserId = "user_id";
const char* const UserStatus = "user_status";
const char* const PinCode = "pin_code";

}  // namespace DoorLockCluster

namespace IasAceCluster {

const char* const PanelStatus = "panel_status";
const char* const SecondsRemaining = "seconds_remaining";
const char* const AlarmStatus = "alarm_status";

}  // namespace IasAceCluster
}  // namespace CommandParams
}  // namespace Resources
}  // namespace Zigbee
}  // namespace HubPlatform
