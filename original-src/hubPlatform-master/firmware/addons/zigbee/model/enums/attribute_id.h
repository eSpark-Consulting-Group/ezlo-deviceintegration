#pragma once

#include <stdint-gcc.h>
#include <string>
#include <functional>

namespace HubPlatform {
namespace Zigbee {

enum class ClusterId;

enum class BasicClusterAttributeId {
    ZCLVersion = 0x0000,
    ApplicationVersion = 0x0001,
    StackVersion = 0x0002,
    HWVersion = 0x0003,
    ManufacturerName = 0x0004,
    ModelIdentifier = 0x0005,
    DateCode = 0x0006,
    PowerSource = 0x0007,
    GenericDeviceClass = 0x0008,
    GenericDeviceType = 0x0009,
    ProductCode = 0x000a,
    ProductURL = 0x000b,
    ManufacturerVersionDetails = 0x000c,
    SerialNumber = 0x000d,
    ProductLabel = 0x000e,
    LocationDescription = 0x0010,
    PhysicalEnvironment = 0x0011,
    DeviceEnabled = 0x0012,
    AlarmMask = 0x0013,
    DisableLocalConfig = 0x0014,
    SWBuildID = 0x4000
};

enum class PollControlClusterAttributeId {
    CheckInInterval = 0x0000,
    LongPollInterval = 0x0001,
    ShortPollInterval = 0x0002,
    FastPollTimeout = 0x0003,
    CheckInIntervalMin = 0x0004,
    LongPollIntervalMin = 0x0005,
    FastPollTimeoutMax = 0x0006
};

enum class PowerConfigurationClusterAttributeId {
    MainsVoltage = 0x0000,
    MainsFrequency = 0x0001,

    MainsAlarmMask = 0x0010,
    MainsVoltageMinThreshold = 0x0011,
    MainsVoltageMaxThreshold = 0x0012,
    MainsVoltageDwellTripPoint = 0x0013,

    BatteryVoltage = 0x0020,
    BatteryPercentageRemaining = 0x0021,

    BatteryManufacturer = 0x0030,
    BatterySize = 0x0031,
    BatteryAHrRating = 0x0032,
    BatteryQuantity = 0x0033,
    BatteryRatedVoltage = 0x0034,
    BatteryAlarmMask = 0x0035,
    BatteryVoltageMinThreshold = 0x0036,
    BatteryVoltageThreshold1 = 0x0037,
    BatteryVoltageThreshold2 = 0x0038,
    BatteryVoltageThreshold3 = 0x0039,
    BatteryPercentageMinThreshold = 0x003a,
    BatteryPercentageThreshold1 = 0x003b,
    BatteryPercentageThreshold2 = 0x003c,
    BatteryPercentageThreshold3 = 0x003d,
    BatteryAlarmState = 0x003e
};

enum class IASZoneClusterAttributeId {
    ZoneState = 0x0000,
    ZoneType = 0x0001,
    ZoneStatus = 0x0002,
};

enum class OnOffClusterAttributeId {
    OnOff = 0x0000,
    GlobalSceneControl = 0x4000,
    OnTime = 0x4001,
    OffWaitTime = 0x4002
};

enum class LevelControlClusterAttributeId {
    CurrentLevel = 0x0000,
    RemainingTime = 0x0001,
    OnOffTransitionTime = 0x0010,
    OnLevel = 0x0011,
    OnTransitionTime = 0x0012,
    OffTransitionTime = 0x0013,
    DefaultMoveRate = 0x0014
};

enum class ElectricalMeasurementClusterAttributeId {
    // Basic Information
    MeasurementType = 0x0000,

    // DC Measurement
    DCVoltage = 0x0100,
    DCVoltageMin = 0x0101,
    DCVoltageMax = 0x0102,
    DCCurrent = 0x0103,
    DCCurrentMin = 0x0104,
    DCCurrentMax = 0x0105,
    DCPower = 0x0106,
    DCPowerMin = 0x0107,
    DCPowerMax = 0x0108,

    // DC Formatting
    DCVoltageMultiplier = 0x0200,
    DCVoltageDivisor = 0x0201,
    DCCurrentMultiplier = 0x0202,
    DCCurrentDivisor = 0x0203,
    DCPowerMultiplier = 0x0204,
    DCPowerDivisor = 0x0205,

    // AC (Non-phase Specific) Measurements
    ACFrequency = 0x0300,
    ACFrequencyMin = 0x0301,
    ACFrequencyMax = 0x0302,
    NeutralCurrent = 0x0303,
    TotalActivePower = 0x0304,
    TotalReactivePower = 0x0305,
    TotalApparentPower = 0x0306,
    Measured1stHarmonicCurrent = 0x0307,
    Measured3rdHarmonicCurrent = 0x0308,
    Measured5thHarmonicCurrent = 0x0309,
    Measured7thHarmonicCurrent = 0x030A,
    Measured9thHarmonicCurrent = 0x030B,
    Measured11thHarmonicCurrent = 0x030C,
    MeasuredPhase1stHarmonicCurrent = 0x030D,
    MeasuredPhase3rdHarmonicCurrent = 0x030E,
    MeasuredPhase5thHarmonicCurrent = 0x030F,
    MeasuredPhase7thHarmonicCurrent = 0x0310,
    MeasuredPhase9thHarmonicCurrent = 0x0311,
    MeasuredPhase11thHarmonicCurrent = 0x0312,

    // AC (Non-phase Specific) Formatting
    ACFrequencyMultiplier = 0x0400,
    ACFrequencyDivisor = 0x0401,
    PowerMultiplier = 0x0402,
    PowerDivisor = 0x0403,
    HarmonicCurrentMultiplier = 0x0404,
    PhaseHarmonicCurrentMultiplier = 0x0405,

    // AC (Single Phase or Phase A) Measurements
    LineCurrent = 0x0501,
    ActiveCurrent = 0x0502,
    ReactiveCurrent = 0x0503,
    RMSVoltage = 0x0505,
    RMSVoltageMin = 0x0506,
    RMSVoltageMax = 0x0507,
    RMSCurrent = 0x0508,
    RMSCurrentMin = 0x0509,
    RMSCurrentMax = 0x050A,
    ActivePower = 0x050B,
    ActivePowerMin = 0x050C,
    ActivePowerMax = 0x050D,
    ReactivePower = 0x050E,
    ApparentPower = 0x050F,
    PowerFactor = 0x0510,
    AverageRMSVoltageMeasurementPeriod = 0x0511,
    AverageRMSOverVoltageCounter = 0x0512,
    AverageRMSUnderVoltageCounter = 0x0513,
    RMSExtremeOverVoltagePeriod = 0x0514,
    RMSExtremeUnderVoltagePeriod = 0x0515,
    RMSVoltageSagPeriod = 0x0516,
    RMSVoltageSwellPeriod = 0x0517,

    // AC Formatting
    ACVoltageMultiplier = 0x0600,
    ACVoltageDivisor = 0x0601,
    ACCurrentMultiplier = 0x0602,
    ACCurrentDivisor = 0x0603,
    ACPowerMultiplier = 0x0604,
    ACPowerDivisor = 0x0605,

    // DC Manufacturer Threshold Alarms
    DCOverloadAlarmsMask = 0x0700,
    DCVoltageOverload = 0x0701,
    DCCurrentOverload = 0x0702,

    // AC Manufacturer Threshold Alarms
    ACAlarmsMask = 0x0800,
    ACVoltageOverload = 0x0801,
    ACCurrentOverload = 0x0802,
    ACActivePowerOverload = 0x0803,
    ACReactivePowerOverload = 0x0804,
    AverageRMSOverVoltage = 0x0805,
    AverageRMSUnderVoltage = 0x0806,
    RMSExtremeOverVoltage = 0x0807,
    RMSExtremeUnderVoltage = 0x0808,
    RMSVoltageSag = 0x0809,
    RMSVoltageSwell = 0x080A,

    // AC Phase B Measurements
    LineCurrentPhB = 0x0901,
    ActiveCurrentPhB = 0x0902,
    ReactiveCurrentPhB = 0x0903,
    RMSVoltagePhB = 0x0905,
    RMSVoltageMinPhB = 0x0906,
    RMSVoltageMaxPhB = 0x0907,
    RMSCurrentPhB = 0x0908,
    RMSCurrentMinPhB = 0x0909,
    RMSCurrentMaxPhB = 0x090A,
    ActivePowerPhB = 0x090B,
    ActivePowerMinPhB = 0x090C,
    ActivePowerMaxPhB = 0x090D,
    ReactivePowerPhB = 0x090E,
    ApparentPowerPhB = 0x090F,
    PowerFactorPhB = 0x0910,
    AverageRMSVoltageMeasurementPeriodPhB = 0x0911,
    AverageRMSOverVoltageCounterPhB = 0x0912,
    AverageRMSUnderVoltageCounterPhB = 0x0913,
    RMSExtremeOverVoltagePeriodPhB = 0x0914,
    RMSExtremeUnderVoltagePeriodPhB = 0x0915,
    RMSVoltageSagPeriodPhB = 0x0916,
    RMSVoltageSwellPeriodPhB = 0x0917,

    // AC Phase C Measurements
    LineCurrentPhC = 0x0A01,
    ActiveCurrentPhC = 0x0A02,
    ReactiveCurrentPhC = 0x0A03,
    RMSVoltagePhC = 0x0A05,
    RMSVoltageMinPhC = 0x0A06,
    RMSVoltageMaxPhC = 0x0A07,
    RMSCurrentPhC = 0x0A08,
    RMSCurrentMinPhC = 0x0A09,
    RMSCurrentMaxPhC = 0x0A0A,
    ActivePowerPhC = 0x0A0B,
    ActivePowerMinPhC = 0x0A0C,
    ActivePowerMaxPhC = 0x0A0D,
    ReactivePowerPhC = 0x0A0E,
    ApparentPowerPhC = 0x0A0F,
    PowerFactorPhC = 0x0A10,
    AverageRMSVoltageMeasurementPeriodPhC = 0x0A11,
    AverageRMSOverVoltageCounterPhC = 0x0A12,
    AverageRMSUnderVoltageCounterPhC = 0x0A13,
    RMSExtremeOverVoltagePeriodPhC = 0x0A14,
    RMSExtremeUnderVoltagePeriodPhC = 0x0A15,
    RMSVoltageSagPeriodPhC = 0x0A16,
    RMSVoltageSwellPeriodPhC = 0x0A17
};

enum class TemperatureMeasurementClusterAttributeId {
    MeasuredValue = 0x0000,
    MinMeasuredValue = 0x0001,
    MaxMeasuredValue = 0x0002,
    Tolerance = 0x0003
};

enum class WaterContentClusterAttributeId {
    MeasuredValue = 0x0000,
    MinMeasuredValue = 0x0001,
    MaxMeasuredValue = 0x0002,
    Tolerance = 0x0003
};

enum class ThermostatClusterAttributeId {
    // Thermostat Information
    LocalTemperature = 0x0000,
    OutdoorTemperature = 0x0001,
    Occupancy = 0x0002,
    AbsMinHeatSetpointLimit = 0x0003,
    AbsMaxHeatSetpointLimit = 0x0004,
    AbsMinCoolSetpointLimit = 0x0005,
    AbsMaxCoolSetpointLimit = 0x0006,
    PICoolingDemand = 0x0007,
    PIHeatingDemand = 0x0008,
    HVACSystemTypeConfiguration = 0x0009,

    // Thermostat Settings
    LocalTemperatureCalibration = 0x0010,
    OccupiedCoolingSetpoint = 0x0011,
    OccupiedHeatingSetpoint = 0x0012,
    UnoccupiedCoolingSetpoint = 0x0013,
    UnoccupiedHeatingSetpoint = 0x0014,
    MinHeatSetpointLimit = 0x0015,
    MaxHeatSetpointLimit = 0x0016,
    MinCoolSetpointLimit = 0x0017,
    MaxCoolSetpointLimit = 0x0018,
    MinSetpointDeadBand = 0x0019,
    RemoteSensing = 0x001a,
    ControlSequenceOfOperation = 0x001b,
    SystemMode = 0x001c,
    AlarmMask = 0x001d,
    ThermostatRunningMode = 0x001e,
    ThermostatRunningState = 0x0029
};

enum class FanControlClusterAttributeId {
    // Thermostat Information
    FanMode = 0x0000,
    FanModeSequence = 0x0001
};

enum class ColorControlClusterAttributeId {
    // Color Information
    CurrentHue = 0x0000,
    CurrentSaturation = 0x0001,
    RemainingTime = 0x0002,
    CurrentX = 0x0003,
    CurrentY = 0x0004,
    DriftCompensation = 0x0005,
    CompensationText = 0x0006,
    ColorTemperatureMireds = 0x0007,
    ColorMode = 0x0008,
    Options = 0x000f,
    EnhancedCurrentHue = 0x4000,
    EnhancedColorMode = 0x4001,
    ColorLoopActive = 0x4002,
    ColorLoopDirection = 0x4003,
    ColorLoopTime = 0x4004,
    ColorLoopStartEnhancedHue = 0x4005,
    ColorLoopStoredEnhancedHue = 0x4006,
    ColorCapabilities = 0x400a,
    ColorTempPhysicalMinMireds = 0x400b,
    ColorTempPhysicalMaxMireds = 0x400c,
    CoupleColorTempToLevelMinMireds = 0x400d,
    StartUpColorTemperatureMireds = 0x4010,

    // Defined Primaries Information
    NumberOfPrimaries = 0x0010,
    Primary1X = 0x0011,
    Primary1Y = 0x0012,
    Primary1Intensity = 0x0013,
    Primary2X = 0x0015,
    Primary2Y = 0x0016,
    Primary2Intensity = 0x0017,
    Primary3X = 0x0019,
    Primary3Y = 0x001a,
    Primary3Intensity = 0x001b,

    // Additional Defined Primaries Information
    Primary4X = 0x0020,
    Primary4Y = 0x0021,
    Primary4Intensity = 0x0022,
    Primary5X = 0x0024,
    Primary5Y = 0x0025,
    Primary5Intensity = 0x0026,
    Primary6X = 0x0028,
    Primary6Y = 0x0029,
    Primary6Intensity = 0x002a,

    // Defined Color Point Settings
    WhitePointX = 0x0030,
    WhitePointY = 0x0031,
    ColorPointRX = 0x0032,
    ColorPointRY = 0x0033,
    ColorPointRIntensity = 0x0034,
    ColorPointGX = 0x0036,
    ColorPointGY = 0x0037,
    ColorPointGIntensity = 0x0038,
    ColorPointBX = 0x003a,
    ColorPointBY = 0x003b,
    ColorPointBIntensity = 0x003c
};

enum class OccupancySensingClusterAttributeId {
    // Occupancy sensor information
    Occupancy = 0x0000,
    OccupancySensorType = 0x0001,
    OccupancySensorTypeBitmap = 0x0002,

    // PIR configuration
    PIROccupiedToUnoccupiedDelay = 0x0010,
    PIRUnoccupiedToOccupiedDelay = 0x0011,
    PIRUnoccupiedToOccupiedThreshold = 0x0012,

    // Ultrasonic configuration
    UltrasonicOccupiedToUnoccupiedDelay = 0x0020,
    UltrasonicUnoccupiedToOccupiedDelay = 0x0021,
    UltrasonicUnoccupiedToOccupiedThreshold = 0x0022,

    // Physical contact configuration
    PhysicalContactOccupiedToUnoccupiedDelay = 0x0030,
    PhysicalContactUnoccupiedToOccupiedDelay = 0x0031,
    PhysicalContactUnoccupiedToOccupiedThreshold = 0x0032
};

enum class IlluminanceMeasurementClusterAttributeId {
    MeasuredValue = 0x0000,
    MinMeasuredValue = 0x0001,
    MaxMeasuredValue = 0x0002,
    Tolerance = 0x0003,
    LightSensorType = 0x0004,
};

enum class DoorLockClusterAttributeId {
    // Basic Information
    LockState = 0x0000,
    LockType = 0x0001,
    ActuatorEnabled = 0x0002,
    DoorState = 0x0003,
    DoorOpenEvents = 0x0004,
    DoorClosedEvents = 0x0005,
    OpenPeriod = 0x0006,

    // User, PIN, Schedule, Log Information
    NumberOfLogRecordsSupported = 0x0010,
    NumberOfTotalUsersSupported = 0x0011,
    NumberOfPINUsersSupported = 0x0012,
    NumberOfRFIDUsersSupported = 0x0013,
    NumberOfWeekDaySchedulesSupportedPerUser = 0x0014,
    NumberOfYearDaySchedulesSupportedPerUser = 0x0015,
    NumberOfHolidaySchedulesSupported = 0x0016,
    MaxPINCodeLength = 0x0017,
    MinPINCodeLength = 0x0018,
    MaxRFIDCodeLength = 0x0019,
    MinRFIDCodeLength = 0x001A,

    // Operational Settings
    EnableLogging = 0x0020,
    Language = 0x0021,
    LEDSettings = 0x0022,
    AutoRelockTime = 0x0023,
    SoundVolume = 0x0024,
    OperatingMode = 0x0025,
    SupportedOperatingModes = 0x0026,
    DefaultConfigurationRegister = 0x0027,
    EnableLocalProgramming = 0x0028,
    EnableOneTouchLocking = 0x0029,
    EnableInsideStatusLED = 0x002A,
    EnablePrivacyModeButton = 0x002B,

    // Security Settings
    WrongCodeEntryLimit = 0x0030,
    UserCodeTemporaryDisableTime = 0x0031,
    SendPINOverTheAir = 0x0032,
    RequirePINforRFOperation = 0x0033,
    SecurityLevel = 0x0034,

    // Alarm and Event Masks
    AlarmMask = 0x0040,
    KeypadOperationEventMask = 0x0041,
    RFOperationEventMask = 0x0042,
    ManualOperationEventMask = 0x0043,
    RFIDOperationEventMask = 0x0044,
    KeypadProgrammingEventMask = 0x0045,
    RFProgrammingEventMask = 0x0046,
    RFIDProgrammingEventMask = 0x0047
};

enum class MultistateInputBasicAttributeId {
    StateText = 0x000E,
    Description = 0x001C,
    NumberOfStates = 0x004A,
    OutOfService = 0x0051,
    PresentValue = 0x0055,
    Reliability = 0x0067,
    StatusFlags = 0x006F,
    ApplicationType = 0x0100
};

const char* toString( ClusterId clusterId, uint16_t attributeId );
uint16_t attributeIdFromString( ClusterId clusterId, const std::string& attributeName );

bool forEachAttributeId( ClusterId clusterId, std::function< bool( uint16_t ) > callback );

}  // namespace Zigbee
}  // namespace HubPlatform
