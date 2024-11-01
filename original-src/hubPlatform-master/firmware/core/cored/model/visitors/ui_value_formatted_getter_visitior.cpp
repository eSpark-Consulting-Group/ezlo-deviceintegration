#include "cored/model/visitors/ui_value_formatted_getter_visitior.h"

#include "cored/model/configuration/configuration.h"
#include "cored/model/values/values.h"

#include "base_utils/unit_converter.h"
#include "base_utils/float_operations.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

UiValueFormattedGetterVisitor::UiValueFormattedGetterVisitor( Configuration& configuration )
: m_configuration( configuration ) {}

UiValueFormattedGetterVisitor::~UiValueFormattedGetterVisitor() = default;

const std::string& UiValueFormattedGetterVisitor::getValue( const Value& value ) {
    m_result.clear();

    value.accept( *this );

    return m_result;
}

void UiValueFormattedGetterVisitor::visit( const IntValue& value ) {
    m_result = std::to_string( value.get() );
}

void UiValueFormattedGetterVisitor::visit( const BoolValue& value ) {
    m_result = value.get() ? "true" : "false";
}

void UiValueFormattedGetterVisitor::visit( const FloatValue& value ) {
    m_result = UnitConverter::floatToFormattedString( value.get() );
}

void UiValueFormattedGetterVisitor::visit( const StringValue& value ) {
    m_result = value.get();
}

void UiValueFormattedGetterVisitor::visit( const RgbValue& value ) {
    m_result = UnitConverter::rgbToFormattedString(
        value.getColor( Color::Red ), value.getColor( Color::Green ), value.getColor( Color::Blue ) );
}

void UiValueFormattedGetterVisitor::visit( const CameraStreamValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const TokenValue& value ) {
    m_result = value.get();
}

void UiValueFormattedGetterVisitor::visit( const ScalableValue& value ) {
    const auto scale = m_configuration.getScaleForValueType( value.getType() );
    m_result = UnitConverter::floatToFormattedString( scale.empty() ? value.getValue() :
                                                                      value.getConvertedValue( scale.c_str() ) );
}

void UiValueFormattedGetterVisitor::visit( const DictionaryValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const ArrayValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const UserCodeValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const ButtonStateValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const UserLockOperationValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const UserCodeActionValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const DailyIntervalValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const WeeklyIntervalValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const SoundInfoValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const ActionValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const ZwaveConfigurationValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const AssociationGroupValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const AssociationValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const DetectableObjectValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const DetectablePersonValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const DetectableQRCodeValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const CameraHotzoneValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const HotzoneMatchValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const GeofenceValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const ShutterSecondPartValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const LedSegmentValue& /*value*/ ) {}

void UiValueFormattedGetterVisitor::visit( const LedEffectValue& /*value*/ ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
