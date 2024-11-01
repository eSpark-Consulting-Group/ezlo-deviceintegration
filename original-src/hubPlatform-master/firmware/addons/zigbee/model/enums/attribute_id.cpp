#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/attribute_id.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

static constexpr uint16_t UnknowAttributeId = 0xFFFF;

template < typename AttributeId >
struct AttributeIdData {
    AttributeId attributeId;
    const char* name;
};

static const AttributeIdData< BasicClusterAttributeId > basicAttributeIdData[] = {
    { BasicClusterAttributeId::ZCLVersion, Resources::Fields::Attribute::Names::Basic::ZCLVersion },
    { BasicClusterAttributeId::ApplicationVersion, Resources::Fields::Attribute::Names::Basic::ApplicationVersion },
    { BasicClusterAttributeId::StackVersion, Resources::Fields::Attribute::Names::Basic::StackVersion },
    { BasicClusterAttributeId::HWVersion, Resources::Fields::Attribute::Names::Basic::HWVersion },
    { BasicClusterAttributeId::ManufacturerName, Resources::Fields::Attribute::Names::Basic::ManufacturerName },
    { BasicClusterAttributeId::ModelIdentifier, Resources::Fields::Attribute::Names::Basic::ModelIdentifier },
    { BasicClusterAttributeId::DateCode, Resources::Fields::Attribute::Names::Basic::DateCode },
    { BasicClusterAttributeId::PowerSource, Resources::Fields::Attribute::Names::Basic::PowerSource },
    { BasicClusterAttributeId::GenericDeviceClass, Resources::Fields::Attribute::Names::Basic::GenericDeviceClass },
    { BasicClusterAttributeId::GenericDeviceType, Resources::Fields::Attribute::Names::Basic::GenericDeviceType },
    { BasicClusterAttributeId::ProductCode, Resources::Fields::Attribute::Names::Basic::ProductCode },
    { BasicClusterAttributeId::ProductURL, Resources::Fields::Attribute::Names::Basic::ProductURL },
    { BasicClusterAttributeId::ManufacturerVersionDetails,
      Resources::Fields::Attribute::Names::Basic::ManufacturerVersionDetails },
    { BasicClusterAttributeId::SerialNumber, Resources::Fields::Attribute::Names::Basic::SerialNumber },
    { BasicClusterAttributeId::ProductLabel, Resources::Fields::Attribute::Names::Basic::ProductLabel },
    { BasicClusterAttributeId::LocationDescription, Resources::Fields::Attribute::Names::Basic::LocationDescription },
    { BasicClusterAttributeId::PhysicalEnvironment, Resources::Fields::Attribute::Names::Basic::PhysicalEnvironment },
    { BasicClusterAttributeId::DeviceEnabled, Resources::Fields::Attribute::Names::Basic::DeviceEnabled },
    { BasicClusterAttributeId::AlarmMask, Resources::Fields::Attribute::Names::Basic::AlarmMask },
    { BasicClusterAttributeId::DisableLocalConfig, Resources::Fields::Attribute::Names::Basic::DisableLocalConfig },
    { BasicClusterAttributeId::SWBuildID, Resources::Fields::Attribute::Names::Basic::SWBuildID },
};

static const AttributeIdData< PollControlClusterAttributeId > pollControlAttributeIdData[] = {
    { PollControlClusterAttributeId::CheckInInterval,
      Resources::Fields::Attribute::Names::PollControl::CheckInInterval },
    { PollControlClusterAttributeId::LongPollInterval,
      Resources::Fields::Attribute::Names::PollControl::LongPollInterval },
    { PollControlClusterAttributeId::ShortPollInterval,
      Resources::Fields::Attribute::Names::PollControl::ShortPollInterval },
    { PollControlClusterAttributeId::FastPollTimeout,
      Resources::Fields::Attribute::Names::PollControl::FastPollTimeout },
    { PollControlClusterAttributeId::CheckInIntervalMin,
      Resources::Fields::Attribute::Names::PollControl::CheckInIntervalMin },
    { PollControlClusterAttributeId::LongPollIntervalMin,
      Resources::Fields::Attribute::Names::PollControl::LongPollIntervalMin },
    { PollControlClusterAttributeId::FastPollTimeoutMax,
      Resources::Fields::Attribute::Names::PollControl::FastPollTimeoutMax }
};

static const AttributeIdData< PowerConfigurationClusterAttributeId > powerConfigurationAttributeIdData[] = {
    { PowerConfigurationClusterAttributeId::MainsVoltage,
      Resources::Fields::Attribute::Names::PowerConfiguration::MainsVoltage },
    { PowerConfigurationClusterAttributeId::MainsFrequency,
      Resources::Fields::Attribute::Names::PowerConfiguration::MainsFrequency },
    { PowerConfigurationClusterAttributeId::MainsAlarmMask,
      Resources::Fields::Attribute::Names::PowerConfiguration::MainsAlarmMask },
    { PowerConfigurationClusterAttributeId::MainsVoltageMinThreshold,
      Resources::Fields::Attribute::Names::PowerConfiguration::MainsVoltageMinThreshold },
    { PowerConfigurationClusterAttributeId::MainsVoltageMaxThreshold,
      Resources::Fields::Attribute::Names::PowerConfiguration::MainsVoltageMaxThreshold },
    { PowerConfigurationClusterAttributeId::MainsVoltageDwellTripPoint,
      Resources::Fields::Attribute::Names::PowerConfiguration::MainsVoltageDwellTripPoint },
    { PowerConfigurationClusterAttributeId::BatteryVoltage,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryVoltage },
    { PowerConfigurationClusterAttributeId::BatteryPercentageRemaining,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryPercentageRemaining },
    { PowerConfigurationClusterAttributeId::BatteryManufacturer,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryManufacturer },
    { PowerConfigurationClusterAttributeId::BatterySize,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatterySize },
    { PowerConfigurationClusterAttributeId::BatteryAHrRating,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryAHrRating },
    { PowerConfigurationClusterAttributeId::BatteryQuantity,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryQuantity },
    { PowerConfigurationClusterAttributeId::BatteryRatedVoltage,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryRatedVoltage },
    { PowerConfigurationClusterAttributeId::BatteryAlarmMask,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryAlarmMask },
    { PowerConfigurationClusterAttributeId::BatteryVoltageMinThreshold,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryVoltageMinThreshold },
    { PowerConfigurationClusterAttributeId::BatteryVoltageThreshold1,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryVoltageThreshold1 },
    { PowerConfigurationClusterAttributeId::BatteryVoltageThreshold2,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryVoltageThreshold2 },
    { PowerConfigurationClusterAttributeId::BatteryVoltageThreshold3,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryVoltageThreshold3 },
    { PowerConfigurationClusterAttributeId::BatteryPercentageMinThreshold,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryPercentageMinThreshold },
    { PowerConfigurationClusterAttributeId::BatteryPercentageThreshold1,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryPercentageThreshold1 },
    { PowerConfigurationClusterAttributeId::BatteryPercentageThreshold2,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryPercentageThreshold2 },
    { PowerConfigurationClusterAttributeId::BatteryPercentageThreshold3,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryPercentageThreshold3 },
    { PowerConfigurationClusterAttributeId::BatteryAlarmState,
      Resources::Fields::Attribute::Names::PowerConfiguration::BatteryAlarmState }

};

static const AttributeIdData< IASZoneClusterAttributeId > iasZoneAttributeIdData[] = {
    { IASZoneClusterAttributeId::ZoneState, Resources::Fields::Attribute::Names::IASZone::ZoneState },
    { IASZoneClusterAttributeId::ZoneType, Resources::Fields::Attribute::Names::IASZone::ZoneType },
    { IASZoneClusterAttributeId::ZoneStatus, Resources::Fields::Attribute::Names::IASZone::ZoneStatus }
};

static const AttributeIdData< OnOffClusterAttributeId > onOffAttributeIdData[] = {
    { OnOffClusterAttributeId::OnOff, Resources::Fields::Attribute::Names::OnOff::OnOff },
    { OnOffClusterAttributeId::GlobalSceneControl, Resources::Fields::Attribute::Names::OnOff::GlobalSceneControl },
    { OnOffClusterAttributeId::OnTime, Resources::Fields::Attribute::Names::OnOff::OnTime },
    { OnOffClusterAttributeId::OffWaitTime, Resources::Fields::Attribute::Names::OnOff::OffWaitTime }
};

static const AttributeIdData< LevelControlClusterAttributeId > levelControlAttributeIdData[] = {
    { LevelControlClusterAttributeId::CurrentLevel, Resources::Fields::Attribute::Names::LevelControl::CurrentLevel },
    { LevelControlClusterAttributeId::RemainingTime, Resources::Fields::Attribute::Names::LevelControl::RemainingTime },
    { LevelControlClusterAttributeId::OnOffTransitionTime,
      Resources::Fields::Attribute::Names::LevelControl::OnOffTransitionTime },
    { LevelControlClusterAttributeId::OnLevel, Resources::Fields::Attribute::Names::LevelControl::OnLevel },
    { LevelControlClusterAttributeId::OnTransitionTime,
      Resources::Fields::Attribute::Names::LevelControl::OnTransitionTime },
    { LevelControlClusterAttributeId::OffTransitionTime,
      Resources::Fields::Attribute::Names::LevelControl::OffTransitionTime },
    { LevelControlClusterAttributeId::DefaultMoveRate,
      Resources::Fields::Attribute::Names::LevelControl::DefaultMoveRate }
};

static const AttributeIdData< ElectricalMeasurementClusterAttributeId > electricalMeasurementAttributeIdData[] = {
    { ElectricalMeasurementClusterAttributeId::MeasurementType,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasurementType },

    { ElectricalMeasurementClusterAttributeId::DCVoltage,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCVoltage },
    { ElectricalMeasurementClusterAttributeId::DCVoltageMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCVoltageMin },
    { ElectricalMeasurementClusterAttributeId::DCVoltageMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCVoltageMax },
    { ElectricalMeasurementClusterAttributeId::DCCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCCurrent },
    { ElectricalMeasurementClusterAttributeId::DCCurrentMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCCurrentMin },
    { ElectricalMeasurementClusterAttributeId::DCCurrentMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCCurrentMax },
    { ElectricalMeasurementClusterAttributeId::DCPower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCPower },
    { ElectricalMeasurementClusterAttributeId::DCPowerMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCPowerMin },
    { ElectricalMeasurementClusterAttributeId::DCPowerMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCPowerMax },

    { ElectricalMeasurementClusterAttributeId::DCVoltageMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCVoltageMultiplier },
    { ElectricalMeasurementClusterAttributeId::DCVoltageDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCVoltageDivisor },
    { ElectricalMeasurementClusterAttributeId::DCCurrentMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCCurrentMultiplier },
    { ElectricalMeasurementClusterAttributeId::DCCurrentDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCCurrentDivisor },
    { ElectricalMeasurementClusterAttributeId::DCPowerMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCPowerMultiplier },
    { ElectricalMeasurementClusterAttributeId::DCPowerDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCPowerDivisor },

    { ElectricalMeasurementClusterAttributeId::ACFrequency,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACFrequency },
    { ElectricalMeasurementClusterAttributeId::ACFrequencyMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACFrequencyMin },
    { ElectricalMeasurementClusterAttributeId::ACFrequencyMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACFrequencyMax },
    { ElectricalMeasurementClusterAttributeId::NeutralCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::NeutralCurrent },
    { ElectricalMeasurementClusterAttributeId::TotalActivePower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::TotalActivePower },
    { ElectricalMeasurementClusterAttributeId::TotalReactivePower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::TotalReactivePower },
    { ElectricalMeasurementClusterAttributeId::TotalApparentPower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::TotalApparentPower },
    { ElectricalMeasurementClusterAttributeId::Measured1stHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::Measured1stHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::Measured3rdHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::Measured3rdHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::Measured5thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::Measured5thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::Measured7thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::Measured7thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::Measured9thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::Measured9thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::Measured11thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::Measured11thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::MeasuredPhase1stHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasuredPhase1stHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::MeasuredPhase3rdHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasuredPhase3rdHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::MeasuredPhase5thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasuredPhase5thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::MeasuredPhase7thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasuredPhase7thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::MeasuredPhase9thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasuredPhase9thHarmonicCurrent },
    { ElectricalMeasurementClusterAttributeId::MeasuredPhase11thHarmonicCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::MeasuredPhase11thHarmonicCurrent },

    { ElectricalMeasurementClusterAttributeId::ACFrequencyMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACFrequencyMultiplier },
    { ElectricalMeasurementClusterAttributeId::ACFrequencyDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACFrequencyDivisor },
    { ElectricalMeasurementClusterAttributeId::PowerMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::PowerMultiplier },
    { ElectricalMeasurementClusterAttributeId::PowerDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::PowerDivisor },
    { ElectricalMeasurementClusterAttributeId::HarmonicCurrentMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::HarmonicCurrentMultiplier },
    { ElectricalMeasurementClusterAttributeId::PhaseHarmonicCurrentMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::PhaseHarmonicCurrentMultiplier },

    { ElectricalMeasurementClusterAttributeId::LineCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::LineCurrent },
    { ElectricalMeasurementClusterAttributeId::ActiveCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActiveCurrent },
    { ElectricalMeasurementClusterAttributeId::ReactiveCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ReactiveCurrent },
    { ElectricalMeasurementClusterAttributeId::RMSVoltage,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltage },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageMin },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageMax },
    { ElectricalMeasurementClusterAttributeId::RMSCurrent,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrent },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentMin },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentMax },
    { ElectricalMeasurementClusterAttributeId::ActivePower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePower },
    { ElectricalMeasurementClusterAttributeId::ActivePowerMin,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerMin },
    { ElectricalMeasurementClusterAttributeId::ActivePowerMax,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerMax },
    { ElectricalMeasurementClusterAttributeId::ReactivePower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ReactivePower },
    { ElectricalMeasurementClusterAttributeId::ApparentPower,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ApparentPower },
    { ElectricalMeasurementClusterAttributeId::PowerFactor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::PowerFactor },
    { ElectricalMeasurementClusterAttributeId::AverageRMSVoltageMeasurementPeriod,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSVoltageMeasurementPeriod },
    { ElectricalMeasurementClusterAttributeId::AverageRMSOverVoltageCounter,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSOverVoltageCounter },
    { ElectricalMeasurementClusterAttributeId::AverageRMSUnderVoltageCounter,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSUnderVoltageCounter },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeOverVoltagePeriod,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeOverVoltagePeriod },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeUnderVoltagePeriod,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeUnderVoltagePeriod },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSagPeriod,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSagPeriod },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSwellPeriod,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSwellPeriod },

    { ElectricalMeasurementClusterAttributeId::ACVoltageMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACVoltageMultiplier },
    { ElectricalMeasurementClusterAttributeId::ACVoltageDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACVoltageDivisor },
    { ElectricalMeasurementClusterAttributeId::ACCurrentMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACCurrentMultiplier },
    { ElectricalMeasurementClusterAttributeId::ACCurrentDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACCurrentDivisor },
    { ElectricalMeasurementClusterAttributeId::ACPowerMultiplier,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACPowerMultiplier },
    { ElectricalMeasurementClusterAttributeId::ACPowerDivisor,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACPowerDivisor },

    { ElectricalMeasurementClusterAttributeId::DCOverloadAlarmsMask,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCOverloadAlarmsMask },
    { ElectricalMeasurementClusterAttributeId::DCVoltageOverload,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCVoltageOverload },
    { ElectricalMeasurementClusterAttributeId::DCCurrentOverload,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::DCCurrentOverload },

    { ElectricalMeasurementClusterAttributeId::ACAlarmsMask,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACAlarmsMask },
    { ElectricalMeasurementClusterAttributeId::ACVoltageOverload,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACVoltageOverload },
    { ElectricalMeasurementClusterAttributeId::ACCurrentOverload,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACCurrentOverload },
    { ElectricalMeasurementClusterAttributeId::ACActivePowerOverload,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACActivePowerOverload },
    { ElectricalMeasurementClusterAttributeId::ACReactivePowerOverload,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ACReactivePowerOverload },
    { ElectricalMeasurementClusterAttributeId::AverageRMSOverVoltage,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSOverVoltage },
    { ElectricalMeasurementClusterAttributeId::AverageRMSUnderVoltage,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSUnderVoltage },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeOverVoltage,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeOverVoltage },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeUnderVoltage,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeUnderVoltage },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSag,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSag },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSwell,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSwell },

    { ElectricalMeasurementClusterAttributeId::LineCurrentPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::LineCurrentPhB },
    { ElectricalMeasurementClusterAttributeId::ActiveCurrentPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActiveCurrentPhB },
    { ElectricalMeasurementClusterAttributeId::ReactiveCurrentPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ReactiveCurrentPhB },
    { ElectricalMeasurementClusterAttributeId::RMSVoltagePhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltagePhB },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageMinPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageMinPhB },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageMaxPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageMaxPhB },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentPhB },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentMinPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentMinPhB },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentMaxPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentMaxPhB },
    { ElectricalMeasurementClusterAttributeId::ActivePowerPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerPhB },
    { ElectricalMeasurementClusterAttributeId::ActivePowerMinPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerMinPhB },
    { ElectricalMeasurementClusterAttributeId::ActivePowerMaxPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerMaxPhB },
    { ElectricalMeasurementClusterAttributeId::ReactivePowerPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ReactivePowerPhB },
    { ElectricalMeasurementClusterAttributeId::ApparentPowerPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ApparentPowerPhB },
    { ElectricalMeasurementClusterAttributeId::PowerFactorPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::PowerFactorPhB },
    { ElectricalMeasurementClusterAttributeId::AverageRMSVoltageMeasurementPeriodPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSVoltageMeasurementPeriodPhB },
    { ElectricalMeasurementClusterAttributeId::AverageRMSOverVoltageCounterPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSOverVoltageCounterPhB },
    { ElectricalMeasurementClusterAttributeId::AverageRMSUnderVoltageCounterPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSUnderVoltageCounterPhB },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeOverVoltagePeriodPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeOverVoltagePeriodPhB },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeUnderVoltagePeriodPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeUnderVoltagePeriodPhB },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSagPeriodPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSagPeriodPhB },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSwellPeriodPhB,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSwellPeriodPhB },

    { ElectricalMeasurementClusterAttributeId::LineCurrentPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::LineCurrentPhC },
    { ElectricalMeasurementClusterAttributeId::ActiveCurrentPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActiveCurrentPhC },
    { ElectricalMeasurementClusterAttributeId::ReactiveCurrentPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ReactiveCurrentPhC },
    { ElectricalMeasurementClusterAttributeId::RMSVoltagePhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltagePhC },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageMinPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageMinPhC },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageMaxPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageMaxPhC },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentPhC },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentMinPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentMinPhC },
    { ElectricalMeasurementClusterAttributeId::RMSCurrentMaxPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSCurrentMaxPhC },
    { ElectricalMeasurementClusterAttributeId::ActivePowerPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerPhC },
    { ElectricalMeasurementClusterAttributeId::ActivePowerMinPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerMinPhC },
    { ElectricalMeasurementClusterAttributeId::ActivePowerMaxPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ActivePowerMaxPhC },
    { ElectricalMeasurementClusterAttributeId::ReactivePowerPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ReactivePowerPhC },
    { ElectricalMeasurementClusterAttributeId::ApparentPowerPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::ApparentPowerPhC },
    { ElectricalMeasurementClusterAttributeId::PowerFactorPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::PowerFactorPhC },
    { ElectricalMeasurementClusterAttributeId::AverageRMSVoltageMeasurementPeriodPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSVoltageMeasurementPeriodPhC },
    { ElectricalMeasurementClusterAttributeId::AverageRMSOverVoltageCounterPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSOverVoltageCounterPhC },
    { ElectricalMeasurementClusterAttributeId::AverageRMSUnderVoltageCounterPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::AverageRMSUnderVoltageCounterPhC },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeOverVoltagePeriodPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeOverVoltagePeriodPhC },
    { ElectricalMeasurementClusterAttributeId::RMSExtremeUnderVoltagePeriodPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSExtremeUnderVoltagePeriodPhC },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSagPeriodPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSagPeriodPhC },
    { ElectricalMeasurementClusterAttributeId::RMSVoltageSwellPeriodPhC,
      Resources::Fields::Attribute::Names::ElectricalMeasurement::RMSVoltageSwellPeriodPhC }
};

static const AttributeIdData< TemperatureMeasurementClusterAttributeId > temperatureMeasurementAttributeIdData[] = {
    { TemperatureMeasurementClusterAttributeId::MeasuredValue,
      Resources::Fields::Attribute::Names::TemperatureMeasurement::MeasuredValue },
    { TemperatureMeasurementClusterAttributeId::MinMeasuredValue,
      Resources::Fields::Attribute::Names::TemperatureMeasurement::MinMeasuredValue },
    { TemperatureMeasurementClusterAttributeId::MaxMeasuredValue,
      Resources::Fields::Attribute::Names::TemperatureMeasurement::MaxMeasuredValue },
    { TemperatureMeasurementClusterAttributeId::Tolerance,
      Resources::Fields::Attribute::Names::TemperatureMeasurement::Tolerance }
};

static const AttributeIdData< WaterContentClusterAttributeId > waterContentAttributeIdData[] = {
    { WaterContentClusterAttributeId::MeasuredValue, Resources::Fields::Attribute::Names::WaterContent::MeasuredValue },
    { WaterContentClusterAttributeId::MinMeasuredValue,
      Resources::Fields::Attribute::Names::WaterContent::MinMeasuredValue },
    { WaterContentClusterAttributeId::MaxMeasuredValue,
      Resources::Fields::Attribute::Names::WaterContent::MaxMeasuredValue },
    { WaterContentClusterAttributeId::Tolerance, Resources::Fields::Attribute::Names::WaterContent::Tolerance }
};

static const AttributeIdData< ThermostatClusterAttributeId > thermostatAttributeIdData[] = {
    { ThermostatClusterAttributeId::LocalTemperature,
      Resources::Fields::Attribute::Names::Thermostat::LocalTemperature },
    { ThermostatClusterAttributeId::OutdoorTemperature,
      Resources::Fields::Attribute::Names::Thermostat::OutdoorTemperature },
    { ThermostatClusterAttributeId::Occupancy, Resources::Fields::Attribute::Names::Thermostat::Occupancy },
    { ThermostatClusterAttributeId::AbsMinHeatSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::AbsMinHeatSetpointLimit },
    { ThermostatClusterAttributeId::AbsMaxHeatSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::AbsMaxHeatSetpointLimit },
    { ThermostatClusterAttributeId::AbsMinCoolSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::AbsMinCoolSetpointLimit },
    { ThermostatClusterAttributeId::AbsMaxCoolSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::AbsMaxCoolSetpointLimit },
    { ThermostatClusterAttributeId::PICoolingDemand, Resources::Fields::Attribute::Names::Thermostat::PICoolingDemand },
    { ThermostatClusterAttributeId::PIHeatingDemand, Resources::Fields::Attribute::Names::Thermostat::PIHeatingDemand },
    { ThermostatClusterAttributeId::HVACSystemTypeConfiguration,
      Resources::Fields::Attribute::Names::Thermostat::HVACSystemTypeConfiguration },

    { ThermostatClusterAttributeId::LocalTemperatureCalibration,
      Resources::Fields::Attribute::Names::Thermostat::LocalTemperatureCalibration },
    { ThermostatClusterAttributeId::OccupiedCoolingSetpoint,
      Resources::Fields::Attribute::Names::Thermostat::OccupiedCoolingSetpoint },
    { ThermostatClusterAttributeId::OccupiedHeatingSetpoint,
      Resources::Fields::Attribute::Names::Thermostat::OccupiedHeatingSetpoint },
    { ThermostatClusterAttributeId::UnoccupiedCoolingSetpoint,
      Resources::Fields::Attribute::Names::Thermostat::UnoccupiedCoolingSetpoint },
    { ThermostatClusterAttributeId::UnoccupiedHeatingSetpoint,
      Resources::Fields::Attribute::Names::Thermostat::UnoccupiedHeatingSetpoint },
    { ThermostatClusterAttributeId::MinHeatSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::MinHeatSetpointLimit },
    { ThermostatClusterAttributeId::MaxHeatSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::MaxHeatSetpointLimit },
    { ThermostatClusterAttributeId::MinCoolSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::MinCoolSetpointLimit },
    { ThermostatClusterAttributeId::MaxCoolSetpointLimit,
      Resources::Fields::Attribute::Names::Thermostat::MaxCoolSetpointLimit },
    { ThermostatClusterAttributeId::MinSetpointDeadBand,
      Resources::Fields::Attribute::Names::Thermostat::MinSetpointDeadBand },
    { ThermostatClusterAttributeId::RemoteSensing, Resources::Fields::Attribute::Names::Thermostat::RemoteSensing },
    { ThermostatClusterAttributeId::ControlSequenceOfOperation,
      Resources::Fields::Attribute::Names::Thermostat::ControlSequenceOfOperation },
    { ThermostatClusterAttributeId::SystemMode, Resources::Fields::Attribute::Names::Thermostat::SystemMode },
    { ThermostatClusterAttributeId::AlarmMask, Resources::Fields::Attribute::Names::Thermostat::AlarmMask },
    { ThermostatClusterAttributeId::ThermostatRunningMode,
      Resources::Fields::Attribute::Names::Thermostat::ThermostatRunningMode },
    { ThermostatClusterAttributeId::ThermostatRunningState,
      Resources::Fields::Attribute::Names::Thermostat::ThermostatRunningState }
};

static const AttributeIdData< FanControlClusterAttributeId > fanControlAttributeIdData[] = {
    { FanControlClusterAttributeId::FanMode, Resources::Fields::Attribute::Names::FanControl::FanMode },
    { FanControlClusterAttributeId::FanModeSequence, Resources::Fields::Attribute::Names::FanControl::FanModeSequence }
};

static const AttributeIdData< ColorControlClusterAttributeId > colorControlAttributeIdData[] = {
    { ColorControlClusterAttributeId::CurrentHue, Resources::Fields::Attribute::Names::ColorControl::CurrentHue },
    { ColorControlClusterAttributeId::CurrentSaturation,
      Resources::Fields::Attribute::Names::ColorControl::CurrentSaturation },
    { ColorControlClusterAttributeId::RemainingTime, Resources::Fields::Attribute::Names::ColorControl::RemainingTime },
    { ColorControlClusterAttributeId::CurrentX, Resources::Fields::Attribute::Names::ColorControl::CurrentX },
    { ColorControlClusterAttributeId::CurrentY, Resources::Fields::Attribute::Names::ColorControl::CurrentY },
    { ColorControlClusterAttributeId::DriftCompensation,
      Resources::Fields::Attribute::Names::ColorControl::DriftCompensation },
    { ColorControlClusterAttributeId::CompensationText,
      Resources::Fields::Attribute::Names::ColorControl::CompensationText },
    { ColorControlClusterAttributeId::ColorTemperatureMireds,
      Resources::Fields::Attribute::Names::ColorControl::ColorTemperatureMireds },
    { ColorControlClusterAttributeId::ColorMode, Resources::Fields::Attribute::Names::ColorControl::ColorMode },
    { ColorControlClusterAttributeId::Options, Resources::Fields::Attribute::Names::ColorControl::Options },
    { ColorControlClusterAttributeId::EnhancedCurrentHue,
      Resources::Fields::Attribute::Names::ColorControl::EnhancedCurrentHue },
    { ColorControlClusterAttributeId::EnhancedColorMode,
      Resources::Fields::Attribute::Names::ColorControl::EnhancedColorMode },
    { ColorControlClusterAttributeId::ColorLoopActive,
      Resources::Fields::Attribute::Names::ColorControl::ColorLoopActive },
    { ColorControlClusterAttributeId::ColorLoopDirection,
      Resources::Fields::Attribute::Names::ColorControl::ColorLoopDirection },
    { ColorControlClusterAttributeId::ColorLoopTime, Resources::Fields::Attribute::Names::ColorControl::ColorLoopTime },
    { ColorControlClusterAttributeId::ColorLoopStartEnhancedHue,
      Resources::Fields::Attribute::Names::ColorControl::ColorLoopStartEnhancedHue },
    { ColorControlClusterAttributeId::ColorLoopStoredEnhancedHue,
      Resources::Fields::Attribute::Names::ColorControl::ColorLoopStoredEnhancedHue },
    { ColorControlClusterAttributeId::ColorCapabilities,
      Resources::Fields::Attribute::Names::ColorControl::ColorCapabilities },
    { ColorControlClusterAttributeId::ColorTempPhysicalMinMireds,
      Resources::Fields::Attribute::Names::ColorControl::ColorTempPhysicalMinMireds },
    { ColorControlClusterAttributeId::ColorTempPhysicalMaxMireds,
      Resources::Fields::Attribute::Names::ColorControl::ColorTempPhysicalMaxMireds },
    { ColorControlClusterAttributeId::CoupleColorTempToLevelMinMireds,
      Resources::Fields::Attribute::Names::ColorControl::CoupleColorTempToLevelMinMireds },
    { ColorControlClusterAttributeId::StartUpColorTemperatureMireds,
      Resources::Fields::Attribute::Names::ColorControl::StartUpColorTemperatureMireds },

    { ColorControlClusterAttributeId::NumberOfPrimaries,
      Resources::Fields::Attribute::Names::ColorControl::NumberOfPrimaries },
    { ColorControlClusterAttributeId::Primary1X, Resources::Fields::Attribute::Names::ColorControl::Primary1X },
    { ColorControlClusterAttributeId::Primary1Y, Resources::Fields::Attribute::Names::ColorControl::Primary1Y },
    { ColorControlClusterAttributeId::Primary1Intensity,
      Resources::Fields::Attribute::Names::ColorControl::Primary1Intensity },
    { ColorControlClusterAttributeId::Primary2X, Resources::Fields::Attribute::Names::ColorControl::Primary2X },
    { ColorControlClusterAttributeId::Primary2Y, Resources::Fields::Attribute::Names::ColorControl::Primary2Y },
    { ColorControlClusterAttributeId::Primary2Intensity,
      Resources::Fields::Attribute::Names::ColorControl::Primary2Intensity },
    { ColorControlClusterAttributeId::Primary3X, Resources::Fields::Attribute::Names::ColorControl::Primary3X },
    { ColorControlClusterAttributeId::Primary3Y, Resources::Fields::Attribute::Names::ColorControl::Primary3Y },
    { ColorControlClusterAttributeId::Primary3Intensity,
      Resources::Fields::Attribute::Names::ColorControl::Primary3Intensity },

    { ColorControlClusterAttributeId::Primary4X, Resources::Fields::Attribute::Names::ColorControl::Primary4X },
    { ColorControlClusterAttributeId::Primary4Y, Resources::Fields::Attribute::Names::ColorControl::Primary4Y },
    { ColorControlClusterAttributeId::Primary4Intensity,
      Resources::Fields::Attribute::Names::ColorControl::Primary4Intensity },
    { ColorControlClusterAttributeId::Primary5X, Resources::Fields::Attribute::Names::ColorControl::Primary5X },
    { ColorControlClusterAttributeId::Primary5Y, Resources::Fields::Attribute::Names::ColorControl::Primary5Y },
    { ColorControlClusterAttributeId::Primary5Intensity,
      Resources::Fields::Attribute::Names::ColorControl::Primary5Intensity },
    { ColorControlClusterAttributeId::Primary6X, Resources::Fields::Attribute::Names::ColorControl::Primary6X },
    { ColorControlClusterAttributeId::Primary6Y, Resources::Fields::Attribute::Names::ColorControl::Primary6Y },
    { ColorControlClusterAttributeId::Primary6Intensity,
      Resources::Fields::Attribute::Names::ColorControl::Primary6Intensity },

    { ColorControlClusterAttributeId::WhitePointX, Resources::Fields::Attribute::Names::ColorControl::WhitePointX },
    { ColorControlClusterAttributeId::WhitePointY, Resources::Fields::Attribute::Names::ColorControl::WhitePointY },
    { ColorControlClusterAttributeId::ColorPointRX, Resources::Fields::Attribute::Names::ColorControl::ColorPointRX },
    { ColorControlClusterAttributeId::ColorPointRY, Resources::Fields::Attribute::Names::ColorControl::ColorPointRY },
    { ColorControlClusterAttributeId::ColorPointRIntensity,
      Resources::Fields::Attribute::Names::ColorControl::ColorPointRIntensity },
    { ColorControlClusterAttributeId::ColorPointGX, Resources::Fields::Attribute::Names::ColorControl::ColorPointGX },
    { ColorControlClusterAttributeId::ColorPointGY, Resources::Fields::Attribute::Names::ColorControl::ColorPointGY },
    { ColorControlClusterAttributeId::ColorPointGIntensity,
      Resources::Fields::Attribute::Names::ColorControl::ColorPointGIntensity },
    { ColorControlClusterAttributeId::ColorPointBX, Resources::Fields::Attribute::Names::ColorControl::ColorPointBX },
    { ColorControlClusterAttributeId::ColorPointBY, Resources::Fields::Attribute::Names::ColorControl::ColorPointBY },
    { ColorControlClusterAttributeId::ColorPointBIntensity,
      Resources::Fields::Attribute::Names::ColorControl::ColorPointBIntensity }
};

static const AttributeIdData< OccupancySensingClusterAttributeId > occupancySensingAttributeIdData[] = {
    { OccupancySensingClusterAttributeId::Occupancy, Resources::Fields::Attribute::Names::OccupancySensing::Occupancy },
    { OccupancySensingClusterAttributeId::OccupancySensorType,
      Resources::Fields::Attribute::Names::OccupancySensing::OccupancySensorType },
    { OccupancySensingClusterAttributeId::OccupancySensorTypeBitmap,
      Resources::Fields::Attribute::Names::OccupancySensing::OccupancySensorTypeBitmap },

    { OccupancySensingClusterAttributeId::PIROccupiedToUnoccupiedDelay,
      Resources::Fields::Attribute::Names::OccupancySensing::PIROccupiedToUnoccupiedDelay },
    { OccupancySensingClusterAttributeId::PIRUnoccupiedToOccupiedDelay,
      Resources::Fields::Attribute::Names::OccupancySensing::PIRUnoccupiedToOccupiedDelay },
    { OccupancySensingClusterAttributeId::PIRUnoccupiedToOccupiedThreshold,
      Resources::Fields::Attribute::Names::OccupancySensing::PIRUnoccupiedToOccupiedThreshold },

    { OccupancySensingClusterAttributeId::UltrasonicOccupiedToUnoccupiedDelay,
      Resources::Fields::Attribute::Names::OccupancySensing::UltrasonicOccupiedToUnoccupiedDelay },
    { OccupancySensingClusterAttributeId::UltrasonicUnoccupiedToOccupiedDelay,
      Resources::Fields::Attribute::Names::OccupancySensing::UltrasonicUnoccupiedToOccupiedDelay },
    { OccupancySensingClusterAttributeId::UltrasonicUnoccupiedToOccupiedThreshold,
      Resources::Fields::Attribute::Names::OccupancySensing::UltrasonicUnoccupiedToOccupiedThreshold },

    { OccupancySensingClusterAttributeId::PhysicalContactOccupiedToUnoccupiedDelay,
      Resources::Fields::Attribute::Names::OccupancySensing::PhysicalContactOccupiedToUnoccupiedDelay },
    { OccupancySensingClusterAttributeId::PhysicalContactUnoccupiedToOccupiedDelay,
      Resources::Fields::Attribute::Names::OccupancySensing::PhysicalContactUnoccupiedToOccupiedDelay },
    { OccupancySensingClusterAttributeId::PhysicalContactUnoccupiedToOccupiedThreshold,
      Resources::Fields::Attribute::Names::OccupancySensing::PhysicalContactUnoccupiedToOccupiedThreshold },
};

static const AttributeIdData< IlluminanceMeasurementClusterAttributeId > illuminanceMeasurementAttributeIdData[] = {
    { IlluminanceMeasurementClusterAttributeId::MeasuredValue,
      Resources::Fields::Attribute::Names::IlluminanceMeasurement::MeasuredValue },
    { IlluminanceMeasurementClusterAttributeId::MinMeasuredValue,
      Resources::Fields::Attribute::Names::IlluminanceMeasurement::MinMeasuredValue },
    { IlluminanceMeasurementClusterAttributeId::MaxMeasuredValue,
      Resources::Fields::Attribute::Names::IlluminanceMeasurement::MaxMeasuredValue },
    { IlluminanceMeasurementClusterAttributeId::Tolerance,
      Resources::Fields::Attribute::Names::IlluminanceMeasurement::Tolerance },
    { IlluminanceMeasurementClusterAttributeId::LightSensorType,
      Resources::Fields::Attribute::Names::IlluminanceMeasurement::LightSensorType }
};

static const AttributeIdData< DoorLockClusterAttributeId > doorLockAttributeIdData[] = {
    { DoorLockClusterAttributeId::LockState, Resources::Fields::Attribute::Names::DoorLock::LockState },
    { DoorLockClusterAttributeId::LockType, Resources::Fields::Attribute::Names::DoorLock::LockType },
    { DoorLockClusterAttributeId::ActuatorEnabled, Resources::Fields::Attribute::Names::DoorLock::ActuatorEnabled },
    { DoorLockClusterAttributeId::DoorState, Resources::Fields::Attribute::Names::DoorLock::DoorState },
    { DoorLockClusterAttributeId::DoorOpenEvents, Resources::Fields::Attribute::Names::DoorLock::DoorOpenEvents },
    { DoorLockClusterAttributeId::DoorClosedEvents, Resources::Fields::Attribute::Names::DoorLock::DoorClosedEvents },
    { DoorLockClusterAttributeId::OpenPeriod, Resources::Fields::Attribute::Names::DoorLock::OpenPeriod },

    { DoorLockClusterAttributeId::NumberOfLogRecordsSupported,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfLogRecordsSupported },
    { DoorLockClusterAttributeId::NumberOfTotalUsersSupported,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfTotalUsersSupported },
    { DoorLockClusterAttributeId::NumberOfPINUsersSupported,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfPINUsersSupported },
    { DoorLockClusterAttributeId::NumberOfRFIDUsersSupported,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfRFIDUsersSupported },
    { DoorLockClusterAttributeId::NumberOfWeekDaySchedulesSupportedPerUser,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfWeekDaySchedulesSupportedPerUser },
    { DoorLockClusterAttributeId::NumberOfYearDaySchedulesSupportedPerUser,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfYearDaySchedulesSupportedPerUser },
    { DoorLockClusterAttributeId::NumberOfHolidaySchedulesSupported,
      Resources::Fields::Attribute::Names::DoorLock::NumberOfHolidaySchedulesSupported },
    { DoorLockClusterAttributeId::MaxPINCodeLength, Resources::Fields::Attribute::Names::DoorLock::MaxPINCodeLength },
    { DoorLockClusterAttributeId::MinPINCodeLength, Resources::Fields::Attribute::Names::DoorLock::MinPINCodeLength },
    { DoorLockClusterAttributeId::MaxRFIDCodeLength, Resources::Fields::Attribute::Names::DoorLock::MaxRFIDCodeLength },
    { DoorLockClusterAttributeId::MinRFIDCodeLength, Resources::Fields::Attribute::Names::DoorLock::MinRFIDCodeLength },

    { DoorLockClusterAttributeId::EnableLogging, Resources::Fields::Attribute::Names::DoorLock::EnableLogging },
    { DoorLockClusterAttributeId::Language, Resources::Fields::Attribute::Names::DoorLock::Language },
    { DoorLockClusterAttributeId::LEDSettings, Resources::Fields::Attribute::Names::DoorLock::LEDSettings },
    { DoorLockClusterAttributeId::AutoRelockTime, Resources::Fields::Attribute::Names::DoorLock::AutoRelockTime },
    { DoorLockClusterAttributeId::SoundVolume, Resources::Fields::Attribute::Names::DoorLock::SoundVolume },
    { DoorLockClusterAttributeId::OperatingMode, Resources::Fields::Attribute::Names::DoorLock::OperatingMode },
    { DoorLockClusterAttributeId::SupportedOperatingModes,
      Resources::Fields::Attribute::Names::DoorLock::SupportedOperatingModes },
    { DoorLockClusterAttributeId::DefaultConfigurationRegister,
      Resources::Fields::Attribute::Names::DoorLock::DefaultConfigurationRegister },
    { DoorLockClusterAttributeId::EnableLocalProgramming,
      Resources::Fields::Attribute::Names::DoorLock::EnableLocalProgramming },
    { DoorLockClusterAttributeId::EnableOneTouchLocking,
      Resources::Fields::Attribute::Names::DoorLock::EnableOneTouchLocking },
    { DoorLockClusterAttributeId::EnableInsideStatusLED,
      Resources::Fields::Attribute::Names::DoorLock::EnableInsideStatusLED },
    { DoorLockClusterAttributeId::EnablePrivacyModeButton,
      Resources::Fields::Attribute::Names::DoorLock::EnablePrivacyModeButton },

    { DoorLockClusterAttributeId::WrongCodeEntryLimit,
      Resources::Fields::Attribute::Names::DoorLock::WrongCodeEntryLimit },
    { DoorLockClusterAttributeId::UserCodeTemporaryDisableTime,
      Resources::Fields::Attribute::Names::DoorLock::UserCodeTemporaryDisableTime },
    { DoorLockClusterAttributeId::SendPINOverTheAir, Resources::Fields::Attribute::Names::DoorLock::SendPINOverTheAir },
    { DoorLockClusterAttributeId::RequirePINforRFOperation,
      Resources::Fields::Attribute::Names::DoorLock::RequirePINforRFOperation },
    { DoorLockClusterAttributeId::SecurityLevel, Resources::Fields::Attribute::Names::DoorLock::SecurityLevel },

    { DoorLockClusterAttributeId::AlarmMask, Resources::Fields::Attribute::Names::DoorLock::AlarmMask },
    { DoorLockClusterAttributeId::KeypadOperationEventMask,
      Resources::Fields::Attribute::Names::DoorLock::KeypadOperationEventMask },
    { DoorLockClusterAttributeId::RFOperationEventMask,
      Resources::Fields::Attribute::Names::DoorLock::RFOperationEventMask },
    { DoorLockClusterAttributeId::ManualOperationEventMask,
      Resources::Fields::Attribute::Names::DoorLock::ManualOperationEventMask },
    { DoorLockClusterAttributeId::RFIDOperationEventMask,
      Resources::Fields::Attribute::Names::DoorLock::RFIDOperationEventMask },
    { DoorLockClusterAttributeId::KeypadProgrammingEventMask,
      Resources::Fields::Attribute::Names::DoorLock::KeypadProgrammingEventMask },
    { DoorLockClusterAttributeId::RFProgrammingEventMask,
      Resources::Fields::Attribute::Names::DoorLock::RFProgrammingEventMask },
    { DoorLockClusterAttributeId::RFIDProgrammingEventMask,
      Resources::Fields::Attribute::Names::DoorLock::RFIDProgrammingEventMask }
};

static const AttributeIdData< MultistateInputBasicAttributeId > multistateInputBasicAttributeIdData[] = {
    { MultistateInputBasicAttributeId::StateText,
      Resources::Fields::Attribute::Names::MultistateInputBasic::StateText },
    { MultistateInputBasicAttributeId::Description,
      Resources::Fields::Attribute::Names::MultistateInputBasic::Description },
    { MultistateInputBasicAttributeId::NumberOfStates,
      Resources::Fields::Attribute::Names::MultistateInputBasic::NumberOfStates },
    { MultistateInputBasicAttributeId::OutOfService,
      Resources::Fields::Attribute::Names::MultistateInputBasic::OutOfService },
    { MultistateInputBasicAttributeId::PresentValue,
      Resources::Fields::Attribute::Names::MultistateInputBasic::PresentValue },
    { MultistateInputBasicAttributeId::Reliability,
      Resources::Fields::Attribute::Names::MultistateInputBasic::Reliability },
    { MultistateInputBasicAttributeId::StatusFlags,
      Resources::Fields::Attribute::Names::MultistateInputBasic::StatusFlags },
    { MultistateInputBasicAttributeId::ApplicationType,
      Resources::Fields::Attribute::Names::MultistateInputBasic::ApplicationType }
};

template < typename AttributeId, size_t SIZE >
const char* toStringInternal( AttributeId attributeId, const AttributeIdData< AttributeId > ( &datas )[ SIZE ] ) {
    for( const auto& data : datas ) {
        if( data.attributeId == attributeId ) {
            return data.name;
        }
    }

    return Resources::Fields::Attribute::Names::Unknown;
}

template < typename AttributeId, size_t SIZE >
uint16_t fromStringInternal( const std::string& attributeName,
                             const AttributeIdData< AttributeId > ( &datas )[ SIZE ] ) {
    for( const auto& data : datas ) {
        if( attributeName == data.name ) {
            return static_cast< uint16_t >( data.attributeId );
        }
    }

    assert( !"Unknown attribute name" );

    return UnknowAttributeId;
}

template < typename AttributeId, size_t SIZE >
bool forEachAttributeIdInternal( const AttributeIdData< AttributeId > ( &datas )[ SIZE ],
                                 std::function< bool( uint16_t ) > callback ) {
    for( const auto& data : datas ) {
        if( !callback( static_cast< uint16_t >( data.attributeId ) ) ) {
            return false;
        }
    }

    return true;
}

const char* toString( ClusterId clusterId, uint16_t attributeId ) {
    switch( clusterId ) {
    case ClusterId::Basic:
        return toStringInternal( static_cast< BasicClusterAttributeId >( attributeId ), basicAttributeIdData );

    case ClusterId::PollControl:
        return toStringInternal( static_cast< PollControlClusterAttributeId >( attributeId ),
                                 pollControlAttributeIdData );

    case ClusterId::PowerConfiguration:
        return toStringInternal( static_cast< PowerConfigurationClusterAttributeId >( attributeId ),
                                 powerConfigurationAttributeIdData );

    case ClusterId::IASZone:
        return toStringInternal( static_cast< IASZoneClusterAttributeId >( attributeId ), iasZoneAttributeIdData );

    case ClusterId::OnOff:
        return toStringInternal( static_cast< OnOffClusterAttributeId >( attributeId ), onOffAttributeIdData );

    case ClusterId::LevelControl:
        return toStringInternal( static_cast< LevelControlClusterAttributeId >( attributeId ),
                                 levelControlAttributeIdData );

    case ClusterId::ElectricalMeasurement:
        return toStringInternal( static_cast< ElectricalMeasurementClusterAttributeId >( attributeId ),
                                 electricalMeasurementAttributeIdData );

    case ClusterId::TemperatureMeasurement:
        return toStringInternal( static_cast< TemperatureMeasurementClusterAttributeId >( attributeId ),
                                 temperatureMeasurementAttributeIdData );

    case ClusterId::IlluminanceMeasurement:
        return toStringInternal( static_cast< IlluminanceMeasurementClusterAttributeId >( attributeId ),
                                 illuminanceMeasurementAttributeIdData );

    case ClusterId::RelativeHumidity:
    case ClusterId::LeafWetness:
    case ClusterId::SoilMoisture:
        return toStringInternal( static_cast< WaterContentClusterAttributeId >( attributeId ),
                                 waterContentAttributeIdData );

    case ClusterId::OccupancySensing:
        return toStringInternal( static_cast< OccupancySensingClusterAttributeId >( attributeId ),
                                 occupancySensingAttributeIdData );

    case ClusterId::Thermostat:
        return toStringInternal( static_cast< ThermostatClusterAttributeId >( attributeId ),
                                 thermostatAttributeIdData );

    case ClusterId::FanControl:
        return toStringInternal( static_cast< FanControlClusterAttributeId >( attributeId ),
                                 fanControlAttributeIdData );

    case ClusterId::ColorControl:
        return toStringInternal( static_cast< ColorControlClusterAttributeId >( attributeId ),
                                 colorControlAttributeIdData );

    case ClusterId::DoorLock:
        return toStringInternal( static_cast< DoorLockClusterAttributeId >( attributeId ), doorLockAttributeIdData );

    case ClusterId::MultistateInputBasic:
        return toStringInternal( static_cast< MultistateInputBasicAttributeId >( attributeId ),
                                 multistateInputBasicAttributeIdData );

    default:
        return Resources::Fields::Attribute::Names::Unknown;
    }
}

uint16_t attributeIdFromString( ClusterId clusterId, const std::string& attributeName ) {
    switch( clusterId ) {
    case ClusterId::Basic:
        return fromStringInternal( attributeName, basicAttributeIdData );

    case ClusterId::PollControl:
        return fromStringInternal( attributeName, pollControlAttributeIdData );

    case ClusterId::PowerConfiguration:
        return fromStringInternal( attributeName, powerConfigurationAttributeIdData );

    case ClusterId::IASZone:
        return fromStringInternal( attributeName, iasZoneAttributeIdData );

    case ClusterId::OnOff:
        return fromStringInternal( attributeName, onOffAttributeIdData );

    case ClusterId::LevelControl:
        return fromStringInternal( attributeName, levelControlAttributeIdData );

    case ClusterId::ElectricalMeasurement:
        return fromStringInternal( attributeName, electricalMeasurementAttributeIdData );

    case ClusterId::TemperatureMeasurement:
        return fromStringInternal( attributeName, temperatureMeasurementAttributeIdData );

    case ClusterId::IlluminanceMeasurement:
        return fromStringInternal( attributeName, illuminanceMeasurementAttributeIdData );

    case ClusterId::RelativeHumidity:
    case ClusterId::LeafWetness:
    case ClusterId::SoilMoisture:
        return fromStringInternal( attributeName, waterContentAttributeIdData );

    case ClusterId::OccupancySensing:
        return fromStringInternal( attributeName, occupancySensingAttributeIdData );

    case ClusterId::Thermostat:
        return fromStringInternal( attributeName, thermostatAttributeIdData );

    case ClusterId::FanControl:
        return fromStringInternal( attributeName, fanControlAttributeIdData );

    case ClusterId::ColorControl:
        return fromStringInternal( attributeName, colorControlAttributeIdData );

    case ClusterId::DoorLock:
        return fromStringInternal( attributeName, doorLockAttributeIdData );

    case ClusterId::MultistateInputBasic:
        return fromStringInternal( attributeName, multistateInputBasicAttributeIdData );
    default:
        assert( !"Unknown attribute name" );
        return UnknowAttributeId;
    }
}

bool forEachAttributeId( ClusterId clusterId, std::function< bool( uint16_t ) > callback ) {
    switch( clusterId ) {
    case ClusterId::Basic:
        return forEachAttributeIdInternal( basicAttributeIdData, callback );

    case ClusterId::PollControl:
        return forEachAttributeIdInternal( pollControlAttributeIdData, callback );

    case ClusterId::PowerConfiguration:
        return forEachAttributeIdInternal( powerConfigurationAttributeIdData, callback );

    case ClusterId::IASZone:
        return forEachAttributeIdInternal( iasZoneAttributeIdData, callback );

    case ClusterId::OnOff:
        return forEachAttributeIdInternal( onOffAttributeIdData, callback );

    case ClusterId::LevelControl:
        return forEachAttributeIdInternal( levelControlAttributeIdData, callback );

    case ClusterId::ElectricalMeasurement:
        return forEachAttributeIdInternal( electricalMeasurementAttributeIdData, callback );

    case ClusterId::TemperatureMeasurement:
        return forEachAttributeIdInternal( temperatureMeasurementAttributeIdData, callback );

    case ClusterId::IlluminanceMeasurement:
        return forEachAttributeIdInternal( illuminanceMeasurementAttributeIdData, callback );

    case ClusterId::RelativeHumidity:
    case ClusterId::LeafWetness:
    case ClusterId::SoilMoisture:
        return forEachAttributeIdInternal( waterContentAttributeIdData, callback );

    case ClusterId::OccupancySensing:
        return forEachAttributeIdInternal( occupancySensingAttributeIdData, callback );

    case ClusterId::Thermostat:
        return forEachAttributeIdInternal( thermostatAttributeIdData, callback );

    case ClusterId::FanControl:
        return forEachAttributeIdInternal( fanControlAttributeIdData, callback );

    case ClusterId::ColorControl:
        return forEachAttributeIdInternal( colorControlAttributeIdData, callback );

    case ClusterId::DoorLock:
        return forEachAttributeIdInternal( doorLockAttributeIdData, callback );

    case ClusterId::MultistateInputBasic:
        return forEachAttributeIdInternal( multistateInputBasicAttributeIdData, callback );

    default:
        return true;
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
