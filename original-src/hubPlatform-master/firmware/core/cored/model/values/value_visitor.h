#pragma once

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;
class IntValue;
class BoolValue;
class FloatValue;
class StringValue;
class RgbValue;
class CameraStreamValue;
class TokenValue;
class ScalableValue;
class DictionaryValue;
class UserCodeValue;
class ButtonStateValue;
class UserLockOperationValue;
class UserCodeActionValue;
class ArrayValue;
class DailyIntervalValue;
class WeeklyIntervalValue;
class SoundInfoValue;
class ActionValue;
class ZwaveConfigurationValue;
class AssociationGroupValue;
class AssociationValue;
class DetectableObjectValue;
class DetectablePersonValue;
class DetectableQRCodeValue;
class CameraHotzoneValue;
class HotzoneMatchValue;
class GeofenceValue;
class ShutterSecondPartValue;
class LedSegmentValue;
class LedEffectValue;

class ValueVisitor {
public:
    virtual ~ValueVisitor() = default;

    virtual void visit( IntValue& value ) = 0;
    virtual void visit( BoolValue& value ) = 0;
    virtual void visit( FloatValue& value ) = 0;
    virtual void visit( StringValue& value ) = 0;
    virtual void visit( RgbValue& value ) = 0;
    virtual void visit( CameraStreamValue& value ) = 0;
    virtual void visit( TokenValue& value ) = 0;
    virtual void visit( ScalableValue& value ) = 0;
    virtual void visit( DictionaryValue& value ) = 0;
    virtual void visit( UserCodeValue& value ) = 0;
    virtual void visit( ButtonStateValue& value ) = 0;
    virtual void visit( UserLockOperationValue& value ) = 0;
    virtual void visit( UserCodeActionValue& value ) = 0;
    virtual void visit( ArrayValue& value ) = 0;
    virtual void visit( DailyIntervalValue& value ) = 0;
    virtual void visit( WeeklyIntervalValue& value ) = 0;
    virtual void visit( SoundInfoValue& value ) = 0;
    virtual void visit( ActionValue& value ) = 0;
    virtual void visit( ZwaveConfigurationValue& value ) = 0;
    virtual void visit( AssociationGroupValue& value ) = 0;
    virtual void visit( AssociationValue& value ) = 0;
    virtual void visit( DetectableObjectValue& value ) = 0;
    virtual void visit( DetectablePersonValue& value ) = 0;
    virtual void visit( DetectableQRCodeValue& value ) = 0;
    virtual void visit( CameraHotzoneValue& value ) = 0;
    virtual void visit( HotzoneMatchValue& value ) = 0;
    virtual void visit( GeofenceValue& value ) = 0;
    virtual void visit( ShutterSecondPartValue& value ) = 0;
    virtual void visit( LedSegmentValue& value ) = 0;
    virtual void visit( LedEffectValue& value ) = 0;
};

class ValueDefaultVisitor : public ValueVisitor {
public:
    void visit( IntValue& value ) override{};
    void visit( BoolValue& value ) override{};
    void visit( FloatValue& value ) override{};
    void visit( StringValue& value ) override{};
    void visit( RgbValue& value ) override{};
    void visit( CameraStreamValue& value ) override{};
    void visit( TokenValue& value ) override{};
    void visit( ScalableValue& value ) override{};
    void visit( DictionaryValue& value ) override{};
    void visit( UserCodeValue& value ) override{};
    void visit( ButtonStateValue& value ) override{};
    void visit( UserLockOperationValue& value ) override{};
    void visit( UserCodeActionValue& value ) override{};
    void visit( ArrayValue& value ) override{};
    void visit( DailyIntervalValue& value ) override{};
    void visit( WeeklyIntervalValue& value ) override{};
    void visit( SoundInfoValue& value ) override{};
    void visit( ActionValue& value ) override{};
    void visit( ZwaveConfigurationValue& value ) override{};
    void visit( AssociationGroupValue& value ) override{};
    void visit( AssociationValue& value ) override{};
    void visit( DetectableObjectValue& value ) override{};
    void visit( DetectablePersonValue& value ) override{};
    void visit( DetectableQRCodeValue& value ) override{};
    void visit( CameraHotzoneValue& value ) override{};
    void visit( HotzoneMatchValue& value ) override{};
    void visit( GeofenceValue& value ) override{};
    void visit( ShutterSecondPartValue& value ) override{};
    void visit( LedSegmentValue& value ) override{};
    void visit( LedEffectValue& value ) override{};
};

class ValueConstVisitor {
public:
    virtual ~ValueConstVisitor() = default;

    virtual void visit( const IntValue& value ) = 0;
    virtual void visit( const BoolValue& value ) = 0;
    virtual void visit( const FloatValue& value ) = 0;
    virtual void visit( const StringValue& value ) = 0;
    virtual void visit( const RgbValue& value ) = 0;
    virtual void visit( const CameraStreamValue& value ) = 0;
    virtual void visit( const TokenValue& value ) = 0;
    virtual void visit( const ScalableValue& value ) = 0;
    virtual void visit( const DictionaryValue& value ) = 0;
    virtual void visit( const UserCodeValue& value ) = 0;
    virtual void visit( const ButtonStateValue& value ) = 0;
    virtual void visit( const UserLockOperationValue& value ) = 0;
    virtual void visit( const UserCodeActionValue& value ) = 0;
    virtual void visit( const ArrayValue& value ) = 0;
    virtual void visit( const DailyIntervalValue& value ) = 0;
    virtual void visit( const WeeklyIntervalValue& value ) = 0;
    virtual void visit( const SoundInfoValue& value ) = 0;
    virtual void visit( const ActionValue& value ) = 0;
    virtual void visit( const ZwaveConfigurationValue& value ) = 0;
    virtual void visit( const AssociationGroupValue& value ) = 0;
    virtual void visit( const AssociationValue& value ) = 0;
    virtual void visit( const DetectableObjectValue& value ) = 0;
    virtual void visit( const DetectablePersonValue& value ) = 0;
    virtual void visit( const DetectableQRCodeValue& value ) = 0;
    virtual void visit( const CameraHotzoneValue& value ) = 0;
    virtual void visit( const HotzoneMatchValue& value ) = 0;
    virtual void visit( const GeofenceValue& value ) = 0;
    virtual void visit( const ShutterSecondPartValue& value ) = 0;
    virtual void visit( const LedSegmentValue& value ) = 0;
    virtual void visit( const LedEffectValue& value ) = 0;
};

class ValueDefaultConstVisitor : public ValueConstVisitor {
public:
    void visit( const IntValue& value ) override{};
    void visit( const BoolValue& value ) override{};
    void visit( const FloatValue& value ) override{};
    void visit( const StringValue& value ) override{};
    void visit( const RgbValue& value ) override{};
    void visit( const CameraStreamValue& value ) override{};
    void visit( const TokenValue& value ) override{};
    void visit( const ScalableValue& value ) override{};
    void visit( const DictionaryValue& value ) override{};
    void visit( const UserCodeValue& value ) override{};
    void visit( const ButtonStateValue& value ) override{};
    void visit( const UserLockOperationValue& value ) override{};
    void visit( const UserCodeActionValue& value ) override{};
    void visit( const ArrayValue& value ) override{};
    void visit( const DailyIntervalValue& value ) override{};
    void visit( const WeeklyIntervalValue& value ) override{};
    void visit( const SoundInfoValue& value ) override{};
    void visit( const ActionValue& value ) override{};
    void visit( const ZwaveConfigurationValue& value ) override{};
    void visit( const AssociationGroupValue& value ) override{};
    void visit( const AssociationValue& value ) override{};
    void visit( const DetectableObjectValue& value ) override{};
    void visit( const DetectablePersonValue& value ) override{};
    void visit( const DetectableQRCodeValue& value ) override{};
    void visit( const CameraHotzoneValue& value ) override{};
    void visit( const HotzoneMatchValue& value ) override{};
    void visit( const GeofenceValue& value ) override{};
    void visit( const ShutterSecondPartValue& value ) override{};
    void visit( const LedSegmentValue& value ) override{};
    void visit( const LedEffectValue& value ) override{};
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
