#include "cored/model/visitors/ui_value_writer_visitor.h"

#include "cored/model/configuration/configuration.h"
#include "cored/model/values/values.h"

#include <cassert>

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace {

void writeScalableValue( const ScalableValue& value,
                         Json& jsonValue,
                         const char* valueKey,
                         const Configuration& configuration ) {
    auto scale = configuration.getScaleForValueType( value.getType() );
    if( scale.empty() ) {
        scale = value.getScale();
    }

    jsonValue[ valueKey ] = value.getConvertedValue( scale.c_str() );
    jsonValue[ "scale" ] = std::move( scale );
}

class SubElementValueWriter : public ValueConstVisitor {
public:
    SubElementValueWriter( Json& jsonValue, const Configuration& configuration )
    : m_jsonValue( jsonValue )
    , m_configuration( configuration ) {}

    void visit( const IntValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const BoolValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const FloatValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const StringValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const RgbValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const CameraStreamValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const TokenValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const ScalableValue& value ) override {
        writeScalableValue( value, m_jsonValue, "value", m_configuration );
    }
    void visit( const DictionaryValue& value ) override {
        m_jsonValue = value.toJson();
    }

    void visit( const ArrayValue& value ) override {
        m_jsonValue = value.toJson();
    }

    void visit( const UserCodeValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const ButtonStateValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const UserLockOperationValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const UserCodeActionValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const DailyIntervalValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const WeeklyIntervalValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const SoundInfoValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const ActionValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const ZwaveConfigurationValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const AssociationGroupValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const AssociationValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const DetectableObjectValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const DetectablePersonValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const DetectableQRCodeValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const CameraHotzoneValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const HotzoneMatchValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const GeofenceValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const ShutterSecondPartValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const LedSegmentValue& value ) override {
        m_jsonValue = value.toJson();
    }
    void visit( const LedEffectValue& value ) override {
        m_jsonValue = value.toJson();
    }

private:
    Json& m_jsonValue;
    const Configuration& m_configuration;
};
}  // namespace

UiValueWriterVisitor::UiValueWriterVisitor( Json& json, const Configuration& configuration )
: m_json( json )
, m_configuration( configuration ) {}

UiValueWriterVisitor::~UiValueWriterVisitor() = default;

void UiValueWriterVisitor::writeValue( const Value& value, const char* key ) {
    assert( key );

    m_key = key;

    auto restoreKey = []( UiValueWriterVisitor* visitor ) { visitor->m_key = nullptr; };

    std::unique_ptr< UiValueWriterVisitor, decltype( restoreKey ) > keyRestoreGuard( this, restoreKey );

    value.accept( *this );
}

void UiValueWriterVisitor::visit( const IntValue& value ) {
    m_json[ m_key ] = value.get();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const BoolValue& value ) {
    m_json[ m_key ] = value.get();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const FloatValue& value ) {
    m_json[ m_key ] = value.get();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const StringValue& value ) {
    m_json[ m_key ] = value.get();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const RgbValue& value ) {
    auto& result = m_json[ m_key ];
    value.forEachComponent( [&result]( Color id, uint8_t val ) {
        result[ colorToString( id ) ] = val;
        return true;
    } );
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const CameraStreamValue& value ) {
    auto& result = m_json[ m_key ];
    result[ "localRtspUrl" ] = value.getLocalRtspUrl();
    result[ "cloudLiveStreamUrl" ] = value.getCloudLiveStreamUrl();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const TokenValue& value ) {
    m_json[ m_key ] = value.get();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const ScalableValue& value ) {
    writeScalableValue( value, m_json, m_key, m_configuration );
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const DictionaryValue& value ) {
    auto& jsonValue = m_json[ m_key ];
    value.forEachValue( [&jsonValue, this]( const DictionaryValue::ValueId& id, const Value& elementValue ) {
        SubElementValueWriter elementWriter( jsonValue[ id ], m_configuration );
        elementValue.accept( elementWriter );
    } );

    m_json[ "valueType" ] = "dictionary";
    m_json[ "elementType" ] = value.getValuesType().toString();
}

void UiValueWriterVisitor::visit( const ArrayValue& value ) {
    Json array = Json::array();
    value.forEachValue( [&array, this]( const Value& elementValue ) {
        array.push_back( Json() );
        SubElementValueWriter elementWriter( array.back(), m_configuration );
        elementValue.accept( elementWriter );
    } );

    m_json[ m_key ] = std::move( array );
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const UserCodeValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const ButtonStateValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const UserLockOperationValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const UserCodeActionValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const DailyIntervalValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const WeeklyIntervalValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const SoundInfoValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const ActionValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const ZwaveConfigurationValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const AssociationGroupValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const AssociationValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const DetectableObjectValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const DetectablePersonValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const DetectableQRCodeValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const CameraHotzoneValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const HotzoneMatchValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const GeofenceValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const ShutterSecondPartValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const LedSegmentValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::visit( const LedEffectValue& value ) {
    m_json[ m_key ] = value.toJson();
    writeDefaultType( value );
}

void UiValueWriterVisitor::writeDefaultType( const Value& value ) {
    m_json[ "valueType" ] = value.getType().toString();
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
