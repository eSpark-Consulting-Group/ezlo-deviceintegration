#include "cored/model/values/value_factory.h"

#include "cored/model/values/values.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"

#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace ValueFactory {

namespace {

struct DefaultValueCreator {
    template < typename Type, typename... Args >
    ValueUPtr create( Args&&... args ) const {
        return Type::createDefaultValue( std::forward< Args >( args )... );
    }
};

struct ValueCreator {
    template < typename Type, typename... Args >
    ValueUPtr create( Args&&... args ) const {
        return Type::createValue( std::forward< Args >( args )..., m_value );
    }

    const Json& m_value;
};

template < typename Creator >
ValueUPtr createItemValueImpl( ValueType valueType, Creator&& creator ) {
    switch( valueType.getMainType() ) {
    case ValueType::Enum::Int:
        return creator.template create< IntValue >();

    case ValueType::Enum::Bool:
        return creator.template create< BoolValue >();

    case ValueType::Enum::Float:
        return creator.template create< FloatValue >();

    case ValueType::Enum::String:
        return creator.template create< StringValue >();

    case ValueType::Enum::Rgb:
        return creator.template create< RgbValue >();

    case ValueType::Enum::CameraStream:
        return creator.template create< CameraStreamValue >();

    case ValueType::Enum::Token:
        return creator.template create< TokenValue >();

    case ValueType::Enum::UserCode:
        return creator.template create< UserCodeValue >();

    case ValueType::Enum::ButtonState:
        return creator.template create< ButtonStateValue >();

    case ValueType::Enum::UserLockOperation:
        return creator.template create< UserLockOperationValue >();

    case ValueType::Enum::UserCodeAction:
        return creator.template create< UserCodeActionValue >();

    case ValueType::Enum::DailyInterval:
        return creator.template create< DailyIntervalValue >();

    case ValueType::Enum::WeeklyInterval:
        return creator.template create< WeeklyIntervalValue >();

    case ValueType::Enum::SoundInfo:
        return creator.template create< SoundInfoValue >();

    case ValueType::Enum::Action:
        return creator.template create< ActionValue >();

    case ValueType::Enum::ZwaveConfiguration:
        return creator.template create< ZwaveConfigurationValue >();

    case ValueType::Enum::AssociationGroup:
        return creator.template create< AssociationGroupValue >();

    case ValueType::Enum::Association:
        return creator.template create< AssociationValue >();

    case ValueType::Enum::DetectableObject:
        return creator.template create< DetectableObjectValue >();

    case ValueType::Enum::DetectablePerson:
        return creator.template create< DetectablePersonValue >();

    case ValueType::Enum::DetectableQRCode:
        return creator.template create< DetectableQRCodeValue >();

    case ValueType::Enum::CameraHotzone:
        return creator.template create< CameraHotzoneValue >();

    case ValueType::Enum::HotzoneMatch:
        return creator.template create< HotzoneMatchValue >();

    case ValueType::Enum::Geofence:
        return creator.template create< GeofenceValue >();

    case ValueType::Enum::ShutterSecondPart:
        return creator.template create< ShutterSecondPartValue >();

    case ValueType::Enum::LedSegment:
        return creator.template create< LedSegmentValue >();

    case ValueType::Enum::LedEffect:
        return creator.template create< LedEffectValue >();

    case ValueType::Enum::Illuminance:
    case ValueType::Enum::Pressure:
    case ValueType::Enum::SubstanceAmount:
    case ValueType::Enum::Power:
    case ValueType::Enum::Velocity:
    case ValueType::Enum::Acceleration:
    case ValueType::Enum::Direction:
    case ValueType::Enum::GeneralPurpose:
    case ValueType::Enum::Acidity:
    case ValueType::Enum::ElectricPotential:
    case ValueType::Enum::ElectricCurrent:
    case ValueType::Enum::Force:
    case ValueType::Enum::Irradiance:
    case ValueType::Enum::Precipitation:
    case ValueType::Enum::Length:
    case ValueType::Enum::Mass:
    case ValueType::Enum::VolumeFlow:
    case ValueType::Enum::Volume:
    case ValueType::Enum::Angle:
    case ValueType::Enum::Frequency:
    case ValueType::Enum::SeismicIntensity:
    case ValueType::Enum::SeismicMagnitude:
    case ValueType::Enum::Ultraviolet:
    case ValueType::Enum::ElectricalResistance:
    case ValueType::Enum::ElectricalConductivity:
    case ValueType::Enum::Loudness:
    case ValueType::Enum::Moisture:
    case ValueType::Enum::Time:
    case ValueType::Enum::RadonConcentration:
    case ValueType::Enum::BloodPressure:
    case ValueType::Enum::Energy:
    case ValueType::Enum::RfSignalStrength:
    case ValueType::Enum::Temperature:
    case ValueType::Enum::Humidity:
    case ValueType::Enum::AirQuality:
    case ValueType::Enum::KiloVoltAmpereHour:
    case ValueType::Enum::ReactivePowerInstant:
    case ValueType::Enum::AmountOfUsefulEnergy:
    case ValueType::Enum::ReactivePowerConsumption:
        return creator.template create< ScalableValue >( valueType );

    case ValueType::Enum::Dictionary:
        return creator.template create< DictionaryValue >( valueType );

    case ValueType::Enum::Array:
        return creator.template create< ArrayValue >( valueType );

    default:
        throw UnexpectedValueType( valueType.getMainType() );
    }
}
}  // namespace

ValueUPtr createItemValue( ValueType valueType, const Json& value ) {
    return createItemValueImpl( std::move( valueType ), ValueCreator{ value } );
}

ValueUPtr createDefaultItemValue( ValueType valueType ) {
    return createItemValueImpl( std::move( valueType ), DefaultValueCreator{} );
}

}  // namespace ValueFactory
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
