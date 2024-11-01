#pragma once

#include "cored/model/values/value_visitor.h"

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Configuration;

class UiValueFormattedGetterVisitor : public ValueConstVisitor {
public:
    explicit UiValueFormattedGetterVisitor( Configuration& configuration );
    ~UiValueFormattedGetterVisitor() override;

    const std::string& getValue( const Value& value );

private:
    void visit( const IntValue& value ) override;
    void visit( const BoolValue& value ) override;
    void visit( const FloatValue& value ) override;
    void visit( const StringValue& value ) override;
    void visit( const RgbValue& value ) override;
    void visit( const CameraStreamValue& value ) override;
    void visit( const TokenValue& value ) override;
    void visit( const ScalableValue& value ) override;
    void visit( const DictionaryValue& value ) override;
    void visit( const ArrayValue& value ) override;
    void visit( const UserCodeValue& value ) override;
    void visit( const ButtonStateValue& value ) override;
    void visit( const UserLockOperationValue& value ) override;
    void visit( const UserCodeActionValue& value ) override;
    void visit( const DailyIntervalValue& value ) override;
    void visit( const WeeklyIntervalValue& value ) override;
    void visit( const SoundInfoValue& value ) override;
    void visit( const ActionValue& value ) override;
    void visit( const ZwaveConfigurationValue& value ) override;
    void visit( const AssociationGroupValue& value ) override;
    void visit( const AssociationValue& value ) override;
    void visit( const DetectableObjectValue& value ) override;
    void visit( const DetectablePersonValue& value ) override;
    void visit( const DetectableQRCodeValue& value ) override;
    void visit( const CameraHotzoneValue& value ) override;
    void visit( const HotzoneMatchValue& value ) override;
    void visit( const GeofenceValue& value ) override;
    void visit( const ShutterSecondPartValue& value ) override;
    void visit( const LedSegmentValue& value ) override;
    void visit( const LedEffectValue& value ) override;

private:
    std::string m_result;
    Configuration& m_configuration;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
