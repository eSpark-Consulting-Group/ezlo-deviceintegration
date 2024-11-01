#pragma once

#include <cstdint>
#include <memory>

namespace HubPlatform {
namespace CoreD {
namespace Model {

using ValueScale = const char*;
using ValueScalesList = std::initializer_list< const ValueScale >;
using ValueConverter = float ( * )( float fromValue, ValueScale from, ValueScale to );

namespace ValueScales {
constexpr ValueScale Percent = "percent";
constexpr ValueScale Lux = "lux";
constexpr ValueScale KiloPascal = "kilo_pascal";
constexpr ValueScale PoundPerSquareInch = "pound_per_square_inch";
constexpr ValueScale InchesOfMercury = "inches_of_mercury";
constexpr ValueScale GramPerCubicMeter = "gram_per_cubic_meter";
constexpr ValueScale MicroGramPerCubicMeter = "micro_gram_per_cubic_meter";
constexpr ValueScale MolePerCubicMeter = "mole_per_cubic_meter";
constexpr ValueScale PartsPerMillion = "parts_per_million";
constexpr ValueScale PartsPerBillion = "parts_per_billion";
constexpr ValueScale MilliGramPerLiter = "milli_gram_per_liter";
constexpr ValueScale ParticlesPerDeciliter = "particles_per_deciliter";
constexpr ValueScale Watt = "watt";
constexpr ValueScale BtuPerHour = "btu_per_hour";
constexpr ValueScale MeterPerSecond = "meter_per_second";
constexpr ValueScale MilePerHour = "mile_per_hour";
constexpr ValueScale MeterPerSquareSecond = "meter_per_square_second";
constexpr ValueScale NoDirection = "no_direction";
constexpr ValueScale East = "east";
constexpr ValueScale South = "south";
constexpr ValueScale West = "west";
constexpr ValueScale North = "north";
constexpr ValueScale Float = "float";
constexpr ValueScale PotentialOfHydrogen = "potential_of_hydrogen";
constexpr ValueScale Volt = "volt";
constexpr ValueScale MilliVolt = "milli_volt";
constexpr ValueScale Ampere = "ampere";
constexpr ValueScale MilliAmpere = "milli_ampere";
constexpr ValueScale Newton = "newton";
constexpr ValueScale WattPerSquareMeter = "watt_per_square_meter";
constexpr ValueScale MilliMeterPerHour = "milli_meter_per_hour";
constexpr ValueScale InchesPerHour = "inches_per_hour";
constexpr ValueScale Meter = "meter";
constexpr ValueScale Feet = "feet";
constexpr ValueScale CentiMeter = "centi_meter";
constexpr ValueScale KiloGram = "kilo_gram";
constexpr ValueScale Pounds = "pounds";
constexpr ValueScale CubicMeterPerHour = "cubic_meter_per_hour";
constexpr ValueScale CubicFeetPerMinute = "cubic_feet_per_minute";
constexpr ValueScale LiterPerHour = "liter_per_hour";
constexpr ValueScale Liter = "liter";
constexpr ValueScale CubicMeter = "cubic_meter";
constexpr ValueScale CubicFeet = "cubic_feet";
constexpr ValueScale Gallons = "gallons";
constexpr ValueScale NorthPoleDegrees = "north_pole_degrees";
constexpr ValueScale ElevationDegrees = "elevation_degrees";
constexpr ValueScale RevolutionsPerMinute = "revolutions_per_minute";
constexpr ValueScale Hertz = "hertz";
constexpr ValueScale KiloHertz = "kilo_hertz";
constexpr ValueScale BreathsPerMinute = "breaths_per_minute";
constexpr ValueScale BeatsPerMinute = "beats_per_minute";
constexpr ValueScale Mercalli = "mercalli";
constexpr ValueScale EuropeanMacroseismic = "european_macroseismic";
constexpr ValueScale Liedu = "liedu";
constexpr ValueScale Shindo = "shindo";
constexpr ValueScale Local = "local";
constexpr ValueScale Moment = "moment";
constexpr ValueScale SurfaceWave = "surface_wave";
constexpr ValueScale BodyWave = "body_wave";
constexpr ValueScale UvIndex = "uv_index";
constexpr ValueScale OhmMeter = "ohm_meter";
constexpr ValueScale SiemensPerMeter = "siemens_per_meter";
constexpr ValueScale Decibel = "decibel";
constexpr ValueScale Iaq = "iaq";
constexpr ValueScale AWeightedDecibels = "a_weighted_decibels";
constexpr ValueScale VolumeWaterContent = "volume_water_content";
constexpr ValueScale Impedance = "impedance";
constexpr ValueScale WaterActivity = "water_activity";
constexpr ValueScale Seconds = "seconds";
constexpr ValueScale BecquerelPerCubicMeter = "becquerel_per_cubic_meter";
constexpr ValueScale PicocuriesPerLiter = "picocuries_per_liter";
constexpr ValueScale Systolic = "systolic";
constexpr ValueScale Diastolic = "diastolic";
constexpr ValueScale Joule = "joule";
constexpr ValueScale DecibelMilliWatt = "decibel_milli_watt";
constexpr ValueScale Celsius = "celsius";
constexpr ValueScale Fahrenheit = "fahrenheit";
constexpr ValueScale KiloVoltAmpereHour = "kilo_volt_ampere_hour";
constexpr ValueScale KiloVoltAmperReactive = "kilo_volt_amper_reactive";
constexpr ValueScale KiloWattHour = "kilo_watt_hour";
constexpr ValueScale KiloVoltAmperReactiveHour = "kilo_volt_amper_reactive_hour";

}  // namespace ValueScales

class ValueType;
using ValueTypeConstPtr = std::shared_ptr< ValueType >;

class ValueType {
public:
    enum class Enum {
        Int,
        Bool,
        Float,
        Rgb,
        UserCode,
        String,
        CameraStream,
        Token,
        ButtonState,
        UserLockOperation,
        UserCodeAction,
        DailyInterval,
        WeeklyInterval,
        Action,
        ZwaveConfiguration,
        AssociationGroup,
        Association,
        DetectableObject,
        DetectablePerson,
        DetectableQRCode,
        CameraHotzone,
        HotzoneMatch,
        Geofence,
        ShutterSecondPart,
        LedSegment,
        LedEffect,

        Illuminance,
        Pressure,
        SubstanceAmount,
        Power,
        Velocity,
        Acceleration,
        Direction,
        GeneralPurpose,
        Acidity,
        ElectricPotential,
        ElectricCurrent,
        Force,
        Irradiance,
        Precipitation,
        Length,
        Mass,
        VolumeFlow,
        Volume,
        Angle,
        Frequency,
        SeismicIntensity,
        SeismicMagnitude,
        Ultraviolet,
        ElectricalResistance,
        ElectricalConductivity,
        Loudness,
        Moisture,
        Time,
        RadonConcentration,
        BloodPressure,
        Energy,
        RfSignalStrength,
        Temperature,
        Humidity,
        AirQuality,
        SoundInfo,
        KiloVoltAmpereHour,
        ReactivePowerInstant,
        AmountOfUsefulEnergy,
        ReactivePowerConsumption,

        SimpleTypesCount,

        Dictionary = SimpleTypesCount,
        Array,

        Count,
        NoValue = Count,
    };

    ValueType( Enum type = Enum::NoValue, ValueTypeConstPtr subType = nullptr );

    static ValueType makeDictionaryType( ValueType subType );

    static ValueType makeArrayType( ValueType subType );

    static ValueType fromString( const std::string& name );

    bool isValid() const;

    bool isDictionary() const;

    bool isArray() const;

    bool isContainer() const;

    std::string toString() const;

    const ValueScalesList& getScales() const;

    ValueConverter getConverter() const;

    Enum getMainType() const;

    ValueTypeConstPtr getSubType() const;

    bool hasType( Enum enumType ) const;

    bool operator==( const ValueType& other ) const;

    bool operator!=( const ValueType& other ) const;

private:
    Enum m_mainType = Enum::NoValue;
    ValueTypeConstPtr m_subType;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
