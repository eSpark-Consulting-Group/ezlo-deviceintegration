#include "base_utils/enum_mapper.h"
#include "cored/model/values/value_type.h"

#include "common_utils/cpp14.h"

#include <cassert>
#include <cstring>
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace {

using ValueTypeName = const char*;

struct ValueData {
    ValueData( ValueTypeName name = nullptr, ValueScalesList scales = {}, ValueConverter converter = nullptr )
    : m_name( name )
    , m_scales( scales )
    , m_converter( converter ) {}

    ValueTypeName m_name;
    ValueScalesList m_scales;
    ValueConverter m_converter;
};

using ValueTypeMapper = EnumMapper< ValueType::Enum, ValueData >;

const ValueTypeMapper& getValueTypes() {
    using namespace ValueScales;

    static const auto IlluminanceScales = { Percent, Lux };
    static const auto PressureScales = { KiloPascal, PoundPerSquareInch, InchesOfMercury };
    static const auto SubstanceAmountScales = { GramPerCubicMeter,     MicroGramPerCubicMeter,
                                                MolePerCubicMeter,     PartsPerMillion,
                                                PartsPerBillion,       MilliGramPerLiter,
                                                ParticlesPerDeciliter, Percent };
    static const auto PowerScales = { Watt, BtuPerHour };
    static const auto VelocityScales = { MeterPerSecond, MilePerHour };
    static const auto AccelerationScales = { MeterPerSquareSecond };
    static const auto DirectionScales = { NoDirection, East, South, West, North };
    static const auto GeneralPurposeScales = { Percent, Float };
    static const auto AcidityScales = { PotentialOfHydrogen };
    static const auto ElectricPotentialScales = { Volt, MilliVolt };
    static const auto ElectricCurrentScales = { Ampere, MilliAmpere };
    static const auto ForceScales = { Newton };
    static const auto IrradianceScales = { WattPerSquareMeter };
    static const auto PrecipitationScales = { MilliMeterPerHour, InchesPerHour };
    static const auto LengthScales = { Meter, Feet, CentiMeter };
    static const auto MassScales = { KiloGram, Pounds };
    static const auto VolumeFlowScales = { CubicMeterPerHour, CubicFeetPerMinute, LiterPerHour };
    static const auto VolumeScales = { Liter, CubicMeter, CubicFeet, Gallons };
    static const auto AngleScales = { Percent, NorthPoleDegrees, ElevationDegrees };
    static const auto FrequencyScales = { RevolutionsPerMinute, Hertz, KiloHertz, BreathsPerMinute, BeatsPerMinute };
    static const auto SeismicIntensityScales = { Mercalli, EuropeanMacroseismic, Liedu, Shindo };
    static const auto SeismicMagnitudeScales = { Local, Moment, SurfaceWave, BodyWave };
    static const auto UltravioletScales = { UvIndex };
    static const auto ElectricalResistanceScales = { OhmMeter };
    static const auto ElectricalConductivityScales = { SiemensPerMeter };
    static const auto LoudnessScales = { Decibel, AWeightedDecibels };
    static const auto MoistureScales = { Percent, VolumeWaterContent, Impedance, WaterActivity };
    static const auto TimeScales = { Seconds };
    static const auto RadonConcentrationScales = { BecquerelPerCubicMeter, PicocuriesPerLiter };
    static const auto BloodPressureScales = { Systolic, Diastolic };
    static const auto EnergyScales = { Joule, KiloWattHour };
    static const auto RfSignalStrengthScales = { Percent, DecibelMilliWatt };
    static const auto TemperatureScales = { Celsius, Fahrenheit };
    static const auto HumidityScales = { Percent, GramPerCubicMeter };
    static const auto AirQualityScales = { Iaq };
    static const auto KiloVoltAmpereHourScales = { KiloVoltAmpereHour };
    static const auto ReactivePowerInstantScales = { KiloVoltAmperReactive };
    static const auto AmountOfUsefulEnergyScales = { KiloWattHour };
    static const auto ReactivePowerConsumptionScales = { KiloVoltAmperReactiveHour };

    static const auto TemperatureConverter = []( float fromValue, ValueScale from, ValueScale to ) {
        if( strcmp( from, Celsius ) == 0 && strcmp( to, Fahrenheit ) == 0 ) {
            return fromValue * 9.0f / 5.0f + 32.0f;
        }

        if( strcmp( from, Fahrenheit ) == 0 && strcmp( to, Celsius ) == 0 ) {
            return ( fromValue - 32 ) * 5.0f / 9.0f;
        }

        if( strcmp( from, to ) == 0 ) {
            return fromValue;
        }

        assert( !"Invalid scale was passed to temperature converter: should be celsius or fahrenheit" );
        return 0.f;
    };

    static const ValueTypeMapper valueTypes = {
        { ValueType::Enum::Int, { "int" } },
        { ValueType::Enum::Bool, { "bool" } },
        { ValueType::Enum::Float, { "float" } },
        { ValueType::Enum::Rgb, { "rgb" } },
        { ValueType::Enum::UserCode, { "userCode" } },
        { ValueType::Enum::String, { "string" } },
        { ValueType::Enum::CameraStream, { "cameraStream" } },
        { ValueType::Enum::Token, { "token" } },
        { ValueType::Enum::ButtonState, { "button_state" } },
        { ValueType::Enum::UserLockOperation, { "user_lock_operation" } },
        { ValueType::Enum::UserCodeAction, { "user_code_action" } },
        { ValueType::Enum::Illuminance, { "illuminance", IlluminanceScales } },
        { ValueType::Enum::Pressure, { "pressure", PressureScales } },
        { ValueType::Enum::SubstanceAmount, { "substance_amount", SubstanceAmountScales } },
        { ValueType::Enum::Power, { "power", PowerScales } },
        { ValueType::Enum::Velocity, { "velocity", VelocityScales } },
        { ValueType::Enum::Acceleration, { "acceleration", AccelerationScales } },
        { ValueType::Enum::Direction, { "direction", DirectionScales } },
        { ValueType::Enum::GeneralPurpose, { "general_purpose", GeneralPurposeScales } },
        { ValueType::Enum::Acidity, { "acidity", AcidityScales } },
        { ValueType::Enum::ElectricPotential, { "electric_potential", ElectricPotentialScales } },
        { ValueType::Enum::ElectricCurrent, { "electric_current", ElectricCurrentScales } },
        { ValueType::Enum::Force, { "force", ForceScales } },
        { ValueType::Enum::Irradiance, { "irradiance", IrradianceScales } },
        { ValueType::Enum::Precipitation, { "precipitation", PrecipitationScales } },
        { ValueType::Enum::Length, { "length", LengthScales } },
        { ValueType::Enum::Mass, { "mass", MassScales } },
        { ValueType::Enum::VolumeFlow, { "volume_flow", VolumeFlowScales } },
        { ValueType::Enum::Volume, { "volume", VolumeScales } },
        { ValueType::Enum::Angle, { "angle", AngleScales } },
        { ValueType::Enum::Frequency, { "frequency", FrequencyScales } },
        { ValueType::Enum::SeismicIntensity, { "seismic_intensity", SeismicIntensityScales } },
        { ValueType::Enum::SeismicMagnitude, { "seismic_magnitude", SeismicMagnitudeScales } },
        { ValueType::Enum::Ultraviolet, { "ultraviolet", UltravioletScales } },
        { ValueType::Enum::ElectricalResistance, { "electrical_resistance", ElectricalResistanceScales } },
        { ValueType::Enum::ElectricalConductivity, { "electrical_conductivity", ElectricalConductivityScales } },
        { ValueType::Enum::Loudness, { "loudness", LoudnessScales } },
        { ValueType::Enum::Moisture, { "moisture", MoistureScales } },
        { ValueType::Enum::Time, { "time", TimeScales } },
        { ValueType::Enum::RadonConcentration, { "radon_concentration", RadonConcentrationScales } },
        { ValueType::Enum::BloodPressure, { "blood_pressure", BloodPressureScales } },
        { ValueType::Enum::Energy, { "energy", EnergyScales } },
        { ValueType::Enum::RfSignalStrength, { "rf_signal_strength", RfSignalStrengthScales } },
        { ValueType::Enum::Temperature, { "temperature", TemperatureScales, TemperatureConverter } },
        { ValueType::Enum::Humidity, { "humidity", HumidityScales } },
        { ValueType::Enum::AirQuality, { "air_quality", AirQualityScales } },
        { ValueType::Enum::Dictionary, { "dictionary" } },
        { ValueType::Enum::Array, { "array" } },
        { ValueType::Enum::DailyInterval, { "daily_interval" } },
        { ValueType::Enum::WeeklyInterval, { "weekly_interval" } },
        { ValueType::Enum::SoundInfo, { "sound_info" } },
        { ValueType::Enum::KiloVoltAmpereHour, { "kilo_volt_ampere_hour", KiloVoltAmpereHourScales } },
        { ValueType::Enum::ReactivePowerInstant, { "reactive_power_instant", ReactivePowerInstantScales } },
        { ValueType::Enum::AmountOfUsefulEnergy, { "amount_of_useful_energy", AmountOfUsefulEnergyScales } },
        { ValueType::Enum::ReactivePowerConsumption, { "reactive_power_consumption", ReactivePowerConsumptionScales } },
        { ValueType::Enum::Action, { "action" } },
        { ValueType::Enum::ZwaveConfiguration, { "zwave_configuration" } },
        { ValueType::Enum::AssociationGroup, { "association_group" } },
        { ValueType::Enum::Association, { "association" } },
        { ValueType::Enum::DetectableObject, { "detectable_object" } },
        { ValueType::Enum::DetectablePerson, { "detectable_person" } },
        { ValueType::Enum::DetectableQRCode, { "detectable_qrcode" } },
        { ValueType::Enum::CameraHotzone, { "camera_hotzone" } },
        { ValueType::Enum::HotzoneMatch, { "hotzone_match" } },
        { ValueType::Enum::Geofence, { "geofence" } },
        { ValueType::Enum::ShutterSecondPart, { "shutter_second_part" } },
        { ValueType::Enum::LedSegment, { "led_segment" } },
        { ValueType::Enum::LedEffect, { "led_effect" } },
    };
    return valueTypes;
}

bool isContainerType( ValueType::Enum type ) {
    return type == ValueType::Enum::Dictionary || type == ValueType::Enum::Array;
}

}  // namespace

ValueType::ValueType( Enum mainType, ValueTypeConstPtr subType ) {
    const auto hasSubType = subType != nullptr;

    if( isContainerType( mainType ) == hasSubType && ( !hasSubType || subType->isValid() ) ) {
        m_mainType = mainType;
        m_subType = std::move( subType );
    }
}

ValueType ValueType::makeDictionaryType( ValueType subType ) {
    return ValueType( Enum::Dictionary, std::make_shared< ValueType >( std::move( subType ) ) );
}

ValueType ValueType::makeArrayType( ValueType subType ) {
    return ValueType( Enum::Array, std::make_shared< ValueType >( std::move( subType ) ) );
}

bool ValueType::isValid() const {
    return m_mainType < Enum::Count;
}

bool ValueType::isDictionary() const {
    return m_mainType == Enum::Dictionary;
}

bool ValueType::isArray() const {
    return m_mainType == Enum::Array;
}

bool ValueType::isContainer() const {
    return isContainerType( m_mainType );
}

ValueType::Enum ValueType::getMainType() const {
    return m_mainType;
}

ValueTypeConstPtr ValueType::getSubType() const {
    return m_subType;
}

bool ValueType::hasType( Enum enumType ) const {
    for( auto type = this; type; type = type->getSubType().get() ) {
        if( type->getMainType() == enumType ) {
            return true;
        }
    }

    return false;
}

std::string ValueType::toString() const {
    std::string result;

    for( const ValueType* type = this;; ) {
        const auto found = getValueTypes().find( type->getMainType() );

        if( found == getValueTypes().end() ) {
            return {};
        }

        result += found->m_name;

        type = type->getSubType().get();

        if( !type ) {
            return result;
        }

        result += ".";
    }
}

ValueType ValueType::fromString( const std::string& name ) {
    static const auto namesToTypes =
        getValueTypes().createReverseMapper( []( const ValueData& data ) { return data.m_name; },
                                             []( ValueTypeName a, ValueTypeName b ) { return strcmp( a, b ) < 0; } );

    ValueType result;
    for( auto it = name.rbegin();; ) {
        auto next = std::find( it, name.rend(), '.' );
        const auto& subname = std::distance( it, next ) == name.size() ? name : std::string( next.base(), it.base() );
        auto found = namesToTypes.find( subname.c_str() );

        if( found == ValueType::Enum::NoValue ) {
            return {};
        }

        result = ValueType( found, result.isValid() ? std::make_shared< ValueType >( std::move( result ) ) : nullptr );

        if( !result.isValid() || next == name.rend() ) {
            return result;
        }

        it = next + 1;
    }
}

bool ValueType::operator==( const ValueType& other ) const {
    return m_mainType == other.m_mainType && ( !m_subType || *m_subType == *other.m_subType );
}

bool ValueType::operator!=( const ValueType& other ) const {
    return !( *this == other );
}

const ValueScalesList& ValueType::getScales() const {
    static const ValueScalesList emptyScales;
    const auto found = getValueTypes().find( getMainType() );
    return found == getValueTypes().end() ? emptyScales : found->m_scales;
}

ValueConverter ValueType::getConverter() const {
    const auto found = getValueTypes().find( getMainType() );
    return found == getValueTypes().end() ? nullptr : found->m_converter;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
