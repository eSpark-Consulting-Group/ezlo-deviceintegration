#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace Zigbee {
namespace Resources {

namespace DB {

extern const std::vector< std::string > BackupTableNames;

namespace TableNames {

extern const char* const Devices;
extern const char* const DevicesReachableInited;

}  // namespace TableNames

namespace Fields {

extern const char* const Id;

}  // namespace Fields
}  // namespace DB

namespace Fields {

namespace Node {

extern const char* const Id;
extern const char* const ModelId;
extern const char* const ManufacturerId;
extern const char* const Security;
extern const char* const Endpoints;
extern const char* const IsReachable;
extern const char* const IsBatteryPowered;
extern const char* const IsSleepy;
extern const char* const Capabilities;
extern const char* const LastUpdateTime;
extern const char* const ReachableStateRecheckTimeout;

namespace Values {

extern const char* const Unknown;

}  // namespace Values

}  // namespace Node

namespace Endpoint {

extern const char* const Id;
extern const char* const ProfileId;
extern const char* const DeviceId;
extern const char* const Clusters;

}  // namespace Endpoint

namespace Cluster {

extern const char* const Id;
extern const char* const EndpointId;
extern const char* const Name;
extern const char* const DataHolder;
extern const char* const Attributes;
extern const char* const Commands;

namespace DataHolders {

extern const char* const Undefined;
extern const char* const Client;
extern const char* const Server;
extern const char* const ClientServer;

}  // namespace DataHolders

namespace Names {

extern const char* const Basic;
extern const char* const PowerConfiguration;
extern const char* const DeviceTemperatureConfiguration;
extern const char* const Identify;
extern const char* const Groups;
extern const char* const Scenes;
extern const char* const OnOff;
extern const char* const OnOffSwitchConfiguration;
extern const char* const LevelControl;
extern const char* const Alarms;
extern const char* const Time;
extern const char* const RSSILocation;
extern const char* const Diagnostics;
extern const char* const PollControl;
extern const char* const PowerProfile;
extern const char* const MeterIdentification;
extern const char* const ElectricalMeasurement;
extern const char* const AnalogInputBasic;
extern const char* const AnalogOutputBasic;
extern const char* const AnalogValueBasic;
extern const char* const BinaryInputBasic;
extern const char* const BinaryOutputBasic;
extern const char* const BinaryValueBasic;
extern const char* const MultistateInputBasic;
extern const char* const MultistateOutputBasic;
extern const char* const MultistateValueBasic;
extern const char* const OtaUpgrade;
extern const char* const DoorLock;
extern const char* const Thermostat;
extern const char* const FanControl;
extern const char* const ColorControl;
extern const char* const IlluminanceMeasurement;
extern const char* const TemperatureMeasurement;
extern const char* const RelativeHumidity;
extern const char* const OccupancySensing;
extern const char* const LeafWetness;
extern const char* const SoilMoisture;
extern const char* const ApplianceEventsAndAlerts;
// Security clusters
extern const char* const IASZone;
extern const char* const IASAce;
extern const char* const IASWd;

extern const char* const Unknown;

}  // namespace Names
}  // namespace Cluster

namespace Attribute {

extern const char* const Id;
extern const char* const ClusterId;
extern const char* const Name;
extern const char* const Type;
extern const char* const IsMfgSpecific;
extern const char* const MfgCode;
extern const char* const Value;
extern const char* const FormattedValue;

namespace ValueType {

extern const char* const NoData;
extern const char* const Bool;
extern const char* const Bitmap8Data;
extern const char* const Bitmap16Data;
extern const char* const Uint8Data;
extern const char* const Uint16Data;
extern const char* const Uint32Data;
extern const char* const Int8Data;
extern const char* const Int16Data;
extern const char* const Enum8Data;
extern const char* const Enum16Data;
extern const char* const StringData;
extern const char* const Eui64Data;
extern const char* const UnknownData;

}  // namespace ValueType

namespace Names {

extern const char* const Unknown;

namespace Basic {

extern const char* const ZCLVersion;
extern const char* const ApplicationVersion;
extern const char* const StackVersion;
extern const char* const HWVersion;
extern const char* const ManufacturerName;
extern const char* const ModelIdentifier;
extern const char* const DateCode;
extern const char* const PowerSource;
extern const char* const GenericDeviceClass;
extern const char* const GenericDeviceType;
extern const char* const ProductCode;
extern const char* const ProductURL;
extern const char* const ManufacturerVersionDetails;
extern const char* const SerialNumber;
extern const char* const ProductLabel;
extern const char* const LocationDescription;
extern const char* const PhysicalEnvironment;
extern const char* const DeviceEnabled;
extern const char* const AlarmMask;
extern const char* const DisableLocalConfig;
extern const char* const SWBuildID;

}  // namespace Basic

namespace PollControl {

extern const char* const CheckInInterval;
extern const char* const LongPollInterval;
extern const char* const ShortPollInterval;
extern const char* const FastPollTimeout;
extern const char* const CheckInIntervalMin;
extern const char* const LongPollIntervalMin;
extern const char* const FastPollTimeoutMax;

}  // namespace PollControl

namespace PowerConfiguration {

extern const char* const MainsVoltage;
extern const char* const MainsFrequency;
extern const char* const MainsAlarmMask;
extern const char* const MainsVoltageMinThreshold;
extern const char* const MainsVoltageMaxThreshold;
extern const char* const MainsVoltageDwellTripPoint;
extern const char* const BatteryVoltage;
extern const char* const BatteryPercentageRemaining;
extern const char* const BatteryManufacturer;
extern const char* const BatterySize;
extern const char* const BatteryAHrRating;
extern const char* const BatteryQuantity;
extern const char* const BatteryRatedVoltage;
extern const char* const BatteryAlarmMask;
extern const char* const BatteryVoltageMinThreshold;
extern const char* const BatteryVoltageThreshold1;
extern const char* const BatteryVoltageThreshold2;
extern const char* const BatteryVoltageThreshold3;
extern const char* const BatteryPercentageMinThreshold;
extern const char* const BatteryPercentageThreshold1;
extern const char* const BatteryPercentageThreshold2;
extern const char* const BatteryPercentageThreshold3;
extern const char* const BatteryAlarmState;

}  // namespace PowerConfiguration

namespace IASZone {

extern const char* const ZoneState;
extern const char* const ZoneType;
extern const char* const ZoneStatus;

}  // namespace IASZone

namespace OnOff {

extern const char* const OnOff;
extern const char* const GlobalSceneControl;
extern const char* const OnTime;
extern const char* const OffWaitTime;

}  // namespace OnOff

namespace LevelControl {

extern const char* const CurrentLevel;
extern const char* const RemainingTime;
extern const char* const OnOffTransitionTime;
extern const char* const OnLevel;
extern const char* const OnTransitionTime;
extern const char* const OffTransitionTime;
extern const char* const DefaultMoveRate;

}  // namespace LevelControl

namespace ElectricalMeasurement {

extern const char* const MeasurementType;

extern const char* const DCVoltage;
extern const char* const DCVoltageMin;
extern const char* const DCVoltageMax;
extern const char* const DCCurrent;
extern const char* const DCCurrentMin;
extern const char* const DCCurrentMax;
extern const char* const DCPower;
extern const char* const DCPowerMin;
extern const char* const DCPowerMax;

extern const char* const DCVoltageMultiplier;
extern const char* const DCVoltageDivisor;
extern const char* const DCCurrentMultiplier;
extern const char* const DCCurrentDivisor;
extern const char* const DCPowerMultiplier;
extern const char* const DCPowerDivisor;

extern const char* const ACFrequency;
extern const char* const ACFrequencyMin;
extern const char* const ACFrequencyMax;
extern const char* const NeutralCurrent;
extern const char* const TotalActivePower;
extern const char* const TotalReactivePower;
extern const char* const TotalApparentPower;
extern const char* const Measured1stHarmonicCurrent;
extern const char* const Measured3rdHarmonicCurrent;
extern const char* const Measured5thHarmonicCurrent;
extern const char* const Measured7thHarmonicCurrent;
extern const char* const Measured9thHarmonicCurrent;
extern const char* const Measured11thHarmonicCurrent;
extern const char* const MeasuredPhase1stHarmonicCurrent;
extern const char* const MeasuredPhase3rdHarmonicCurrent;
extern const char* const MeasuredPhase5thHarmonicCurrent;
extern const char* const MeasuredPhase7thHarmonicCurrent;
extern const char* const MeasuredPhase9thHarmonicCurrent;
extern const char* const MeasuredPhase11thHarmonicCurrent;

extern const char* const ACFrequencyMultiplier;
extern const char* const ACFrequencyDivisor;
extern const char* const PowerMultiplier;
extern const char* const PowerDivisor;
extern const char* const HarmonicCurrentMultiplier;
extern const char* const PhaseHarmonicCurrentMultiplier;

extern const char* const LineCurrent;
extern const char* const ActiveCurrent;
extern const char* const ReactiveCurrent;
extern const char* const RMSVoltage;
extern const char* const RMSVoltageMin;
extern const char* const RMSVoltageMax;
extern const char* const RMSCurrent;
extern const char* const RMSCurrentMin;
extern const char* const RMSCurrentMax;
extern const char* const ActivePower;
extern const char* const ActivePowerMin;
extern const char* const ActivePowerMax;
extern const char* const ReactivePower;
extern const char* const ApparentPower;
extern const char* const PowerFactor;
extern const char* const AverageRMSVoltageMeasurementPeriod;
extern const char* const AverageRMSOverVoltageCounter;
extern const char* const AverageRMSUnderVoltageCounter;
extern const char* const RMSExtremeOverVoltagePeriod;
extern const char* const RMSExtremeUnderVoltagePeriod;
extern const char* const RMSVoltageSagPeriod;
extern const char* const RMSVoltageSwellPeriod;

extern const char* const ACVoltageMultiplier;
extern const char* const ACVoltageDivisor;
extern const char* const ACCurrentMultiplier;
extern const char* const ACCurrentDivisor;
extern const char* const ACPowerMultiplier;
extern const char* const ACPowerDivisor;

extern const char* const DCOverloadAlarmsMask;
extern const char* const DCVoltageOverload;
extern const char* const DCCurrentOverload;

extern const char* const ACAlarmsMask;
extern const char* const ACVoltageOverload;
extern const char* const ACCurrentOverload;
extern const char* const ACActivePowerOverload;
extern const char* const ACReactivePowerOverload;
extern const char* const AverageRMSOverVoltage;
extern const char* const AverageRMSUnderVoltage;
extern const char* const RMSExtremeOverVoltage;
extern const char* const RMSExtremeUnderVoltage;
extern const char* const RMSVoltageSag;
extern const char* const RMSVoltageSwell;

extern const char* const LineCurrentPhB;
extern const char* const ActiveCurrentPhB;
extern const char* const ReactiveCurrentPhB;
extern const char* const RMSVoltagePhB;
extern const char* const RMSVoltageMinPhB;
extern const char* const RMSVoltageMaxPhB;
extern const char* const RMSCurrentPhB;
extern const char* const RMSCurrentMinPhB;
extern const char* const RMSCurrentMaxPhB;
extern const char* const ActivePowerPhB;
extern const char* const ActivePowerMinPhB;
extern const char* const ActivePowerMaxPhB;
extern const char* const ReactivePowerPhB;
extern const char* const ApparentPowerPhB;
extern const char* const PowerFactorPhB;
extern const char* const AverageRMSVoltageMeasurementPeriodPhB;
extern const char* const AverageRMSOverVoltageCounterPhB;
extern const char* const AverageRMSUnderVoltageCounterPhB;
extern const char* const RMSExtremeOverVoltagePeriodPhB;
extern const char* const RMSExtremeUnderVoltagePeriodPhB;
extern const char* const RMSVoltageSagPeriodPhB;
extern const char* const RMSVoltageSwellPeriodPhB;

extern const char* const LineCurrentPhC;
extern const char* const ActiveCurrentPhC;
extern const char* const ReactiveCurrentPhC;
extern const char* const RMSVoltagePhC;
extern const char* const RMSVoltageMinPhC;
extern const char* const RMSVoltageMaxPhC;
extern const char* const RMSCurrentPhC;
extern const char* const RMSCurrentMinPhC;
extern const char* const RMSCurrentMaxPhC;
extern const char* const ActivePowerPhC;
extern const char* const ActivePowerMinPhC;
extern const char* const ActivePowerMaxPhC;
extern const char* const ReactivePowerPhC;
extern const char* const ApparentPowerPhC;
extern const char* const PowerFactorPhC;
extern const char* const AverageRMSVoltageMeasurementPeriodPhC;
extern const char* const AverageRMSOverVoltageCounterPhC;
extern const char* const AverageRMSUnderVoltageCounterPhC;
extern const char* const RMSExtremeOverVoltagePeriodPhC;
extern const char* const RMSExtremeUnderVoltagePeriodPhC;
extern const char* const RMSVoltageSagPeriodPhC;
extern const char* const RMSVoltageSwellPeriodPhC;

}  // namespace ElectricalMeasurement

namespace TemperatureMeasurement {

extern const char* const MeasuredValue;
extern const char* const MinMeasuredValue;
extern const char* const MaxMeasuredValue;
extern const char* const Tolerance;

}  // namespace TemperatureMeasurement

namespace WaterContent {

extern const char* const MeasuredValue;
extern const char* const MinMeasuredValue;
extern const char* const MaxMeasuredValue;
extern const char* const Tolerance;

}  // namespace WaterContent

namespace OccupancySensing {

extern const char* const Occupancy;
extern const char* const OccupancySensorType;
extern const char* const OccupancySensorTypeBitmap;

extern const char* const PIROccupiedToUnoccupiedDelay;
extern const char* const PIRUnoccupiedToOccupiedDelay;
extern const char* const PIRUnoccupiedToOccupiedThreshold;

extern const char* const UltrasonicOccupiedToUnoccupiedDelay;
extern const char* const UltrasonicUnoccupiedToOccupiedDelay;
extern const char* const UltrasonicUnoccupiedToOccupiedThreshold;

extern const char* const PhysicalContactOccupiedToUnoccupiedDelay;
extern const char* const PhysicalContactUnoccupiedToOccupiedDelay;
extern const char* const PhysicalContactUnoccupiedToOccupiedThreshold;

}  // namespace OccupancySensing

namespace IlluminanceMeasurement {

extern const char* const MeasuredValue;
extern const char* const MinMeasuredValue;
extern const char* const MaxMeasuredValue;
extern const char* const Tolerance;
extern const char* const LightSensorType;

}  // namespace IlluminanceMeasurement

namespace DoorLock {

extern const char* const LockState;
extern const char* const LockType;
extern const char* const ActuatorEnabled;
extern const char* const DoorState;
extern const char* const DoorOpenEvents;
extern const char* const DoorClosedEvents;
extern const char* const OpenPeriod;

extern const char* const NumberOfLogRecordsSupported;
extern const char* const NumberOfTotalUsersSupported;
extern const char* const NumberOfPINUsersSupported;
extern const char* const NumberOfRFIDUsersSupported;
extern const char* const NumberOfWeekDaySchedulesSupportedPerUser;
extern const char* const NumberOfYearDaySchedulesSupportedPerUser;
extern const char* const NumberOfHolidaySchedulesSupported;
extern const char* const MaxPINCodeLength;
extern const char* const MinPINCodeLength;
extern const char* const MaxRFIDCodeLength;
extern const char* const MinRFIDCodeLength;

extern const char* const EnableLogging;
extern const char* const Language;
extern const char* const LEDSettings;
extern const char* const AutoRelockTime;
extern const char* const SoundVolume;
extern const char* const OperatingMode;
extern const char* const SupportedOperatingModes;
extern const char* const DefaultConfigurationRegister;
extern const char* const EnableLocalProgramming;
extern const char* const EnableOneTouchLocking;
extern const char* const EnableInsideStatusLED;
extern const char* const EnablePrivacyModeButton;

extern const char* const WrongCodeEntryLimit;
extern const char* const UserCodeTemporaryDisableTime;
extern const char* const SendPINOverTheAir;
extern const char* const RequirePINforRFOperation;
extern const char* const SecurityLevel;

extern const char* const AlarmMask;
extern const char* const KeypadOperationEventMask;
extern const char* const RFOperationEventMask;
extern const char* const ManualOperationEventMask;
extern const char* const RFIDOperationEventMask;
extern const char* const KeypadProgrammingEventMask;
extern const char* const RFProgrammingEventMask;
extern const char* const RFIDProgrammingEventMask;

}  // namespace DoorLock

namespace Thermostat {

extern const char* const LocalTemperature;
extern const char* const OutdoorTemperature;
extern const char* const Occupancy;
extern const char* const AbsMinHeatSetpointLimit;
extern const char* const AbsMaxHeatSetpointLimit;
extern const char* const AbsMinCoolSetpointLimit;
extern const char* const AbsMaxCoolSetpointLimit;
extern const char* const PICoolingDemand;
extern const char* const PIHeatingDemand;
extern const char* const HVACSystemTypeConfiguration;

extern const char* const LocalTemperatureCalibration;
extern const char* const OccupiedCoolingSetpoint;
extern const char* const OccupiedHeatingSetpoint;
extern const char* const UnoccupiedCoolingSetpoint;
extern const char* const UnoccupiedHeatingSetpoint;
extern const char* const MinHeatSetpointLimit;
extern const char* const MaxHeatSetpointLimit;
extern const char* const MinCoolSetpointLimit;
extern const char* const MaxCoolSetpointLimit;
extern const char* const MinSetpointDeadBand;
extern const char* const RemoteSensing;
extern const char* const ControlSequenceOfOperation;
extern const char* const SystemMode;
extern const char* const AlarmMask;
extern const char* const ThermostatRunningMode;
extern const char* const ThermostatRunningState;

}  // namespace Thermostat

namespace FanControl {

extern const char* const FanMode;
extern const char* const FanModeSequence;

}  // namespace FanControl

namespace ColorControl {

extern const char* const CurrentHue;
extern const char* const CurrentSaturation;
extern const char* const RemainingTime;
extern const char* const CurrentX;
extern const char* const CurrentY;
extern const char* const DriftCompensation;
extern const char* const CompensationText;
extern const char* const ColorTemperatureMireds;
extern const char* const ColorMode;
extern const char* const Options;
extern const char* const EnhancedCurrentHue;
extern const char* const EnhancedColorMode;
extern const char* const ColorLoopActive;
extern const char* const ColorLoopDirection;
extern const char* const ColorLoopTime;
extern const char* const ColorLoopStartEnhancedHue;
extern const char* const ColorLoopStoredEnhancedHue;
extern const char* const ColorCapabilities;
extern const char* const ColorTempPhysicalMinMireds;
extern const char* const ColorTempPhysicalMaxMireds;
extern const char* const CoupleColorTempToLevelMinMireds;
extern const char* const StartUpColorTemperatureMireds;

extern const char* const NumberOfPrimaries;
extern const char* const Primary1X;
extern const char* const Primary1Y;
extern const char* const Primary1Intensity;
extern const char* const Primary2X;
extern const char* const Primary2Y;
extern const char* const Primary2Intensity;
extern const char* const Primary3X;
extern const char* const Primary3Y;
extern const char* const Primary3Intensity;

extern const char* const Primary4X;
extern const char* const Primary4Y;
extern const char* const Primary4Intensity;
extern const char* const Primary5X;
extern const char* const Primary5Y;
extern const char* const Primary5Intensity;
extern const char* const Primary6X;
extern const char* const Primary6Y;
extern const char* const Primary6Intensity;

extern const char* const WhitePointX;
extern const char* const WhitePointY;
extern const char* const ColorPointRX;
extern const char* const ColorPointRY;
extern const char* const ColorPointRIntensity;
extern const char* const ColorPointGX;
extern const char* const ColorPointGY;
extern const char* const ColorPointGIntensity;
extern const char* const ColorPointBX;
extern const char* const ColorPointBY;
extern const char* const ColorPointBIntensity;

}  // namespace ColorControl

namespace MultistateInputBasic {

extern const char* const StateText;
extern const char* const Description;
extern const char* const NumberOfStates;
extern const char* const OutOfService;
extern const char* const PresentValue;
extern const char* const Reliability;
extern const char* const StatusFlags;
extern const char* const ApplicationType;

}  // namespace MultistateInputBasic
}  // namespace Names

namespace Formats {

extern const char* const Undefined;
extern const char* const Unknown;

namespace IASZoneCluster {

extern const char* const Alarm1;
extern const char* const Alarm2;

namespace ZoneState {

extern const char* const NotEnrolled;
extern const char* const Enrolled;

}  // namespace ZoneState

namespace ZoneType {

extern const char* const ZoneType;

extern const char* const StandardCie;
extern const char* const MotionSensor;
extern const char* const ContactSwitch;
extern const char* const FireSensor;
extern const char* const WaterSensor;
extern const char* const GasSensor;
extern const char* const PersonalEmergencyDevice;
extern const char* const VibrationMovementSensor;
extern const char* const RemoteControl;
extern const char* const KeyFob;
extern const char* const Keypad;

extern const char* const SystemAlarm;
extern const char* const IntrusionIndication;
extern const char* const PresenceIndication;
extern const char* const FirstPortal;
extern const char* const SecondPortal;
extern const char* const FireIndication;
extern const char* const WaterOverflow;
extern const char* const CoIndication;
extern const char* const CoocingIndication;
extern const char* const Fall;
extern const char* const Button;
extern const char* const MovementIndication;
extern const char* const Vibration;
extern const char* const Panic;
extern const char* const Emergency;

}  // namespace ZoneType

namespace ZoneStatus {

extern const char* const Tamper;
extern const char* const Battery;
extern const char* const SupervisionReports;
extern const char* const RestoreReports;
extern const char* const Trouble;
extern const char* const Ac;

extern const char* const On;
extern const char* const Off;
extern const char* const Tampered;
extern const char* const NotTampered;
extern const char* const Low;
extern const char* const Ok;
extern const char* const Yes;
extern const char* const No;
extern const char* const Fault;

}  // namespace ZoneStatus
}  // namespace IASZoneCluster

namespace ThermostatCluster {

extern const char* const Off;
extern const char* const Cool;
extern const char* const Heat;

namespace SystemMode {

extern const char* const Auto;
extern const char* const EmergencyHeating;
extern const char* const Precooling;
extern const char* const FanOnly;
extern const char* const Dry;
extern const char* const Sleep;

}  // namespace SystemMode

namespace ThermostatRunningState {

extern const char* const Fan;
extern const char* const Heat2ndStage;
extern const char* const Cool2ndStage;
extern const char* const Fan2ndStage;
extern const char* const Fan3rdStage;

}  // namespace ThermostatRunningState
}  // namespace ThermostatCluster

namespace FanControlCluster {

extern const char* const Off;
extern const char* const Low;
extern const char* const Medium;
extern const char* const High;
extern const char* const On;
extern const char* const Auto;

}  // namespace FanControlCluster

namespace PowerConfigurationCluster {
namespace BatterySize {

extern const char* const NoBattery;
extern const char* const BuiltIn;
extern const char* const Other;
extern const char* const AA;
extern const char* const AAA;
extern const char* const C;
extern const char* const D;
extern const char* const CR2;
extern const char* const CR123A;

}  // namespace BatterySize
}  // namespace PowerConfigurationCluster

namespace ColorControlCluster {
namespace ColorMode {

extern const char* const ColorHueSaturation;
extern const char* const ColorXY;
extern const char* const ColorTemperature;

}  // namespace ColorMode
namespace ColorCapabilities {

extern const char* const HueSaturationSupported;
extern const char* const EnhancedHueSupported;
extern const char* const ColorLoopSupported;
extern const char* const XyAttributesSupported;
extern const char* const ColorTemperatureSupported;

}  // namespace ColorCapabilities
}  // namespace ColorControlCluster

namespace DoorLockCluster {
namespace LockState {

extern const char* const NotFullyLocked;
extern const char* const Locked;
extern const char* const Unlocked;

}  // namespace LockState

namespace LockType {

extern const char* const DeadBolt;
extern const char* const Magnetic;
extern const char* const Other;
extern const char* const Mortise;
extern const char* const Rim;
extern const char* const LatchBolt;
extern const char* const CylindricalLock;
extern const char* const TubularLock;
extern const char* const InterconnectedLock;
extern const char* const DeadLatch;
extern const char* const DoorFurniture;

}  // namespace LockType

namespace DoorLockActuatorState {

extern const char* const Disabled;
extern const char* const Enabled;

}  // namespace DoorLockActuatorState

namespace DoorState {

extern const char* const Open;
extern const char* const Closed;
extern const char* const ErrorJammed;
extern const char* const ErrorForcedOpen;
extern const char* const ErrorUnspecified;

}  // namespace DoorState
}  // namespace DoorLockCluster

namespace OccupancySensingCluster {
namespace Occupancy {

extern const char* const Unoccupied;
extern const char* const Occupied;

}  // namespace Occupancy

namespace OccupancySensorType {

extern const char* const Pir;
extern const char* const Ultrasonic;
extern const char* const PirAndUltrasonic;
extern const char* const PhysicalContact;

}  // namespace OccupancySensorType
}  // namespace OccupancySensingCluster
}  // namespace Formats
}  // namespace Attribute

namespace Command {

extern const char* const Id;
extern const char* const ClusterId;
extern const char* const Name;
extern const char* const Type;
extern const char* const IsMfgSpecific;
extern const char* const MfgCode;

namespace CommandType {

extern const char* const Generated;
extern const char* const Received;
extern const char* const Unknown;

}  // namespace CommandType

namespace NodeType {

extern const char* const Unknown;
extern const char* const Coordinator;
extern const char* const Router;
extern const char* const EndDevice;
extern const char* const SleepyEndDevice;

}  // namespace NodeType

namespace Names {

extern const char* const Unknown;

namespace IasAce {

// Server
extern const char* const ArmResponse;
extern const char* const GetZoneIDMapResponse;
extern const char* const GetZoneInformationResponse;
extern const char* const ZoneStatusChanged;
extern const char* const PanelStatusChanged;
extern const char* const GetPanelStatusResponse;
extern const char* const SetBypassedZoneList;
extern const char* const BypassResponse;
extern const char* const GetZoneStatusResponse;

// Client
extern const char* const Arm;
extern const char* const Bypass;
extern const char* const Emergency;
extern const char* const Fire;
extern const char* const Panic;
extern const char* const GetZoneIDMap;
extern const char* const GetZoneInformation;
extern const char* const GetPanelStatus;
extern const char* const GetBypassedZoneList;
extern const char* const GetZoneStatus;

}  // namespace IasAce

namespace PollControl {

// Server
extern const char* const CheckIn;

// Client
extern const char* const CheckInResponse;
extern const char* const FastPollStop;
extern const char* const SetLongPollInterval;
extern const char* const SetShortPollInterval;

}  // namespace PollControl

namespace OnOff {
// Client
extern const char* const Off;
extern const char* const On;
extern const char* const Toggle;
extern const char* const OffWithEffect;
extern const char* const OnWithRecallGlobalScene;
extern const char* const OnWithTimedOff;

}  // namespace OnOff

namespace LevelControl {
// Client
extern const char* const MoveToLevel;
extern const char* const Move;
extern const char* const Step;
extern const char* const Stop;
extern const char* const MoveToLevelWithOnOff;
extern const char* const MoveWithOnOff;
extern const char* const StepWithOnOff;
extern const char* const StopWithOnOff;
extern const char* const MoveToClosestFrequency;

}  // namespace LevelControl

namespace ColorControl {
// Client
extern const char* const MoveToHue;
extern const char* const MoveHue;
extern const char* const StepHue;
extern const char* const MoveToSaturation;
extern const char* const MoveSaturation;
extern const char* const StepSaturation;
extern const char* const MoveToHueAndSaturation;
extern const char* const MoveToColor;
extern const char* const MoveColor;
extern const char* const StepColor;
extern const char* const MoveToColorTemperature;
extern const char* const EnhancedMoveToHue;
extern const char* const EnhancedMoveHue;
extern const char* const EnhancedStepHue;
extern const char* const EnhancedMoveToHueAndSaturation;
extern const char* const ColorLoopSet;
extern const char* const StopMoveStep;
extern const char* const MoveColorTemperature;
extern const char* const StepColorTemperature;

}  // namespace ColorControl

namespace ApplianceEventsAndAlerts {
extern const char* const AlertsNotification;
extern const char* const GetAlertsResponse;
}  // namespace ApplianceEventsAndAlerts

namespace DoorLock {
// Server
extern const char* const LockDoorResponse;
extern const char* const UnlockDoorResponse;
extern const char* const ToggleResponse;
extern const char* const UnlockWithTimeoutResponse;
extern const char* const GetLogRecordResponse;
extern const char* const SetPinCodeResponse;
extern const char* const GetPinCodeResponse;
extern const char* const ClearPinCodeResponse;
extern const char* const ClearAllPinCodesResponse;
extern const char* const SetUserStatusResponse;
extern const char* const GetUserStatusResponse;
extern const char* const SetWeekdayScheduleResponse;
extern const char* const GetWeekdayScheduleResponse;
extern const char* const ClearWeekdayScheduleResponse;
extern const char* const SetYearDayScheduleResponse;
extern const char* const GetYearDayScheduleResponse;
extern const char* const ClearYearDayScheduleResponse;
extern const char* const SetHolidayScheduleResponse;
extern const char* const GetHolidayScheduleResponse;
extern const char* const ClearHolidayScheduleResponse;
extern const char* const SetUserTypeResponse;
extern const char* const GetUserTypeResponse;
extern const char* const SetRfidCodeResponse;
extern const char* const GetRfidCodeResponse;
extern const char* const ClearRfidCodeResponse;
extern const char* const ClearAllRfidCodesResponse;
extern const char* const OperatingEventNotification;
extern const char* const ProgrammingEventNotification;

// Client
extern const char* const LockDoor;
extern const char* const UnlockDoor;
extern const char* const Toggle;
extern const char* const UnlockWithTimeout;
extern const char* const GetLogRecord;
extern const char* const SetPinCode;
extern const char* const GetPinCode;
extern const char* const ClearPinCode;
extern const char* const ClearAllPinCodes;
extern const char* const SetUserStatus;
extern const char* const GetUserStatus;
extern const char* const SetWeekdaySchedule;
extern const char* const GetWeekdaySchedule;
extern const char* const ClearWeekdaySchedule;
extern const char* const SetYearDaySchedule;
extern const char* const GetYearDaySchedule;
extern const char* const ClearYearDaySchedule;
extern const char* const SetHolidaySchedule;
extern const char* const GetHolidaySchedule;
extern const char* const ClearHolidaySchedule;
extern const char* const SetUserType;
extern const char* const GetUserType;
extern const char* const SetRfidCode;
extern const char* const GetRfidCode;
extern const char* const ClearRfidCode;
extern const char* const ClearAllRfidCodes;

}  // namespace DoorLock
}  // namespace Names

namespace Formats {

namespace AlertStructure {
extern const char* const Reserved;
namespace Category {
extern const char* const Warning;
extern const char* const Danger;
extern const char* const Failure;
extern const char* const Reserved;
}  // namespace Category
namespace PresenceRecovery {
extern const char* const Recovery;
extern const char* const Presence;
extern const char* const Reserved;
}  // namespace PresenceRecovery
}  // namespace AlertStructure

extern const char* const Unknown;

namespace IasAce {
namespace ArmMode {

extern const char* const Disarm;
extern const char* const ArmDayHomeZonesOnly;
extern const char* const ArmNightSleepZonesOnly;
extern const char* const ArmAllZones;

}  // namespace ArmMode

namespace ArmResponse {

extern const char* const AllZonesDisarmed;
extern const char* const OnlyDayHomeZonesArmed;
extern const char* const OnlyNightSleepZonesArmed;
extern const char* const AllZonesArmed;
extern const char* const InvalidArmDisarmCode;
extern const char* const NotReadyToArm;
extern const char* const AlreadyDisarmed;

}  // namespace ArmResponse

namespace PanelStatus {

extern const char* const PanelDisarmed;
extern const char* const ArmedStay;
extern const char* const ArmedNight;
extern const char* const ArmedAway;
extern const char* const ExitDelay;
extern const char* const EntryDelay;
extern const char* const NotReadyToArm;
extern const char* const InAlarm;
extern const char* const ArmingStay;
extern const char* const ArmingNight;
extern const char* const ArmingAway;

}  // namespace PanelStatus

namespace AlarmStatus {

extern const char* const NoAlarm;
extern const char* const Burglar;
extern const char* const Fire;
extern const char* const Emergency;
extern const char* const PolicePanic;
extern const char* const FirePanic;
extern const char* const EmergencyPanic;

}  // namespace AlarmStatus
}  // namespace IasAce

namespace DoorLock {

extern const char* const NotSupported;

namespace UserStatus {

extern const char* const Available;
extern const char* const OccupiedEnabled;
extern const char* const OccupiedDisabled;

}  // namespace UserStatus

namespace UserType {

extern const char* const UnrestrictedUser;
extern const char* const YearDayScheduleUser;
extern const char* const WeekDayScheduleUser;
extern const char* const MasterUser;
extern const char* const NonAccessUser;

}  // namespace UserType

namespace EventSource {

extern const char* const Keypad;
extern const char* const RF;
extern const char* const ManualOrReserved;
extern const char* const RFID;
extern const char* const Indeterminate;

}  // namespace EventSource

namespace OperationEvent {

extern const char* const UnknownOrMfgSpecific;
extern const char* const Lock;
extern const char* const Unlock;
extern const char* const LockFailureInvalidPINorID;
extern const char* const LockFailureInvalidSchedule;
extern const char* const UnlockFailureInvalidPINorID;
extern const char* const UnlockFailureInvalidSchedule;
extern const char* const OneTouchLock;
extern const char* const KeyLock;
extern const char* const KeyUnlock;
extern const char* const AutoLock;
extern const char* const ScheduleLock;
extern const char* const ScheduleUnlock;
extern const char* const ManualLock;
extern const char* const ManualUnlock;
extern const char* const NonAccessUserOperationalEvent;

}  // namespace OperationEvent

namespace ProgrammingEvent {

extern const char* const UnknownOrMfgSpecific;
extern const char* const MasterCodeChanged;
extern const char* const PINCodeAdded;
extern const char* const PINCodeDeleted;
extern const char* const PINCodeChanged;
extern const char* const RFIDCodeAdded;
extern const char* const RFIDCodeDeleted;

}  // namespace ProgrammingEvent
}  // namespace DoorLock
}  // namespace Formats
}  // namespace Command

namespace NetworkInfo {

extern const char* const NodeId;
extern const char* const PanId;
extern const char* const RadioPower;
extern const char* const RadioChannel;
extern const char* const EpanId;
extern const char* const Euid64;
extern const char* const GlobalKey;
extern const char* const GlobalFc;
extern const char* const NetworkKey;
extern const char* const NetworkFc;
extern const char* const NetworkKeySeqNumber;

namespace ChildInfo {

namespace Type {

extern const char* const Unknown;
extern const char* const Coordinator;
extern const char* const Router;
extern const char* const EndDevice;
extern const char* const SleepyEndDevice;

}  // namespace Type

namespace Timeout {

extern const char* const TenSec;
extern const char* const Minutes;

}  // namespace Timeout
}  // namespace ChildInfo

namespace RouteInfo {

namespace ConcentratorType {

extern const char* const Unknown;
extern const char* const Low;
extern const char* const High;

}  // namespace ConcentratorType

namespace Status {

extern const char* const Active;
extern const char* const Discovering;
extern const char* const Unknown;
extern const char* const Unused;

}  // namespace Status
}  // namespace RouteInfo
}  // namespace NetworkInfo
}  // namespace Fields

namespace ScriptEvent {
namespace Param {

extern const char* const NodeId;
extern const char* const EndpointId;
extern const char* const ClusterId;
extern const char* const ClusterName;
extern const char* const Attribute;
extern const char* const AttributeId;
extern const char* const AttributeName;
extern const char* const ManufacturerId;
extern const char* const ModelId;
extern const char* const Status;
extern const char* const ArmMode;
extern const char* const PinCode;
extern const char* const UserId;
extern const char* const UserStatus;
extern const char* const UserType;
extern const char* const EventSource;
extern const char* const EventName;
extern const char* const CommandId;
extern const char* const CommandName;
extern const char* const Value;
extern const char* const AlertId;
extern const char* const Category;
extern const char* const PresenceRecovery;
extern const char* const ManufacturerSpecificBits;

namespace Color {

extern const char* const Wwhite;
extern const char* const Cwhite;
extern const char* const Red;
extern const char* const Green;
extern const char* const Blue;
extern const char* const Amber;
extern const char* const Cyan;
extern const char* const Purple;

}  // namespace Color

namespace ActionStatus {

extern const char* const Invoked;
extern const char* const Started;
extern const char* const Failed;
extern const char* const Timeout;
extern const char* const Finished;
extern const char* const InProgress;
extern const char* const Busy;
extern const char* const Empty;
extern const char* const Undefined;

}  // namespace ActionStatus

namespace ChildAndRouteInfo {

extern const char* const ControllerNodeId;
extern const char* const Type;
extern const char* const Timeout;
extern const char* const Children;
extern const char* const Lqi;
extern const char* const Age;
extern const char* const InCost;
extern const char* const OutCost;
extern const char* const Neighbors;
extern const char* const To;
extern const char* const Through;
extern const char* const ConcentratorType;
extern const char* const Routes;

}  // namespace ChildAndRouteInfo
}  // namespace Param

namespace Name {

extern const char* const NodeAdded;
extern const char* const NodeRejoined;
extern const char* const MfgInterviewFinished;
extern const char* const NodeRemoved;
extern const char* const AttributeValueUpdated;
extern const char* const UnknownAttributeValueUpdated;
extern const char* const MfgSpecificAttributeValueUpdated;
extern const char* const IncludeDevice;
extern const char* const IncludeDeviceProgress;
extern const char* const UserCodesRequestProgress;
extern const char* const ModuleReset;
extern const char* const SimpleCommandReceived;
extern const char* const ArmCommandReceived;
extern const char* const GetUserResponseCommandReceived;
extern const char* const DoorLockOperatingEventNotificationReceived;
extern const char* const DoorLockProgrammingEventNotificationReceived;
extern const char* const ReachableStateUpdated;
extern const char* const AlertReceived;
extern const char* const EmptyAlertsListReceived;
extern const char* const NetworkInfoUpdated;
extern const char* const ChildAndRouteInfoCollected;

}  // namespace Name
}  // namespace ScriptEvent

namespace RPCParams {

extern const char* const NodeId;
extern const char* const EndpointId;
extern const char* const ClusterId;
extern const char* const ClusterName;
extern const char* const AttributeName;
extern const char* const AttributeId;
extern const char* const MinInterval;
extern const char* const MaxInterval;
extern const char* const ReportableChange;
extern const char* const CommandName;
extern const char* const CommandParams;
extern const char* const Value;
extern const char* const MfgCode;
extern const char* const StartUserId;
extern const char* const EndUserId;
extern const char* const InterviewConfigurationsFile;

}  // namespace RPCParams

namespace CommandParams {
namespace LevelControlCluster {

extern const char* const UpParam;
extern const char* const DownParam;

}  // namespace LevelControlCluster

namespace ColorControlCluster {

extern const char* const CurrentX;
extern const char* const CurrentY;

}  // namespace ColorControlCluster

namespace PollControlCluster {

extern const char* const StartFastPollingMode;
extern const char* const FastPollTimeout;

}  // namespace PollControlCluster

namespace DoorLockCluster {

extern const char* const UserId;
extern const char* const UserStatus;
extern const char* const PinCode;

}  // namespace DoorLockCluster

namespace IasAceCluster {

extern const char* const PanelStatus;
extern const char* const SecondsRemaining;
extern const char* const AlarmStatus;

}  // namespace IasAceCluster
}  // namespace CommandParams
}  // namespace Resources
}  // namespace Zigbee
}  // namespace HubPlatform
