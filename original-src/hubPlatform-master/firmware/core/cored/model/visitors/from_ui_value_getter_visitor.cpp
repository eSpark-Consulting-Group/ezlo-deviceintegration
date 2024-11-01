#include "cored/model/visitors/from_ui_value_getter_visitor.h"

#include "cored/model/values/values.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/configuration/configuration.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

FromUiValueGetterVisitor::FromUiValueGetterVisitor( const Configuration& configuration )
: m_jsNewValue( nullptr )
, m_configuration( configuration ) {}

FromUiValueGetterVisitor::~FromUiValueGetterVisitor() = default;

ValueUPtr FromUiValueGetterVisitor::getValue( const Value& oldValue, const Json& newValue ) {
    auto resetState = []( FromUiValueGetterVisitor* self ) { self->m_jsNewValue = nullptr; };
    std::unique_ptr< FromUiValueGetterVisitor, decltype( resetState ) > scopeGuard( this, resetState );

    m_jsNewValue = &newValue;

    oldValue.accept( *this );

    return std::move( m_newValue );
}

void FromUiValueGetterVisitor::visit( const IntValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Int, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const BoolValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Bool, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const FloatValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Float, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const StringValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::String, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const RgbValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Rgb, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const CameraStreamValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::CameraStream, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const TokenValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Token, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const ScalableValue& value ) {
    auto scale = m_configuration.getScaleForValueType( value.getType() );
    if( scale.empty() ) {
        scale = value.getScale();
    }

    m_newValue = ValueFactory::createItemValue( value.getType(), JSON( "value" << *m_jsNewValue << "scale" << scale ) );
}

void FromUiValueGetterVisitor::visit( const DictionaryValue& value ) {
    m_newValue = ValueFactory::createItemValue( value.getType(), *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const ArrayValue& value ) {
    m_newValue = ValueFactory::createItemValue( value.getType(), *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const UserCodeValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::UserCode, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const ButtonStateValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::ButtonState, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const UserLockOperationValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::UserLockOperation, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const UserCodeActionValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::UserCodeAction, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const ShutterSecondPartValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::ShutterSecondPart, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const LedSegmentValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::LedSegment, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const LedEffectValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::LedEffect, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const DailyIntervalValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::DailyInterval, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const WeeklyIntervalValue& /*value*/ ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::WeeklyInterval, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const SoundInfoValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::SoundInfo, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const ActionValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Action, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const ZwaveConfigurationValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::ZwaveConfiguration, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const AssociationGroupValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::AssociationGroup, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const AssociationValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::Association, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const DetectableObjectValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::DetectableObject, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const DetectablePersonValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::DetectablePerson, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const DetectableQRCodeValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::DetectableQRCode, *m_jsNewValue );
}

void FromUiValueGetterVisitor::visit( const CameraHotzoneValue& value ) {
    m_newValue = ValueFactory::createItemValue( ValueType::Enum::CameraHotzone, *m_jsNewValue );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
