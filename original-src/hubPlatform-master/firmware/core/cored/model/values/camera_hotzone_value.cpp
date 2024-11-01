#include "cored/model/values/camera_hotzone_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

#include <unordered_set>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< CameraHotzoneValue > CameraHotzoneValue::createValue( const Json& value ) {
    using namespace Resources;

    auto throwOnError = [&value]( bool status ) {
        if( !status )
            throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::CameraHotzone };
    };

    throwOnError( value.is_object() );
    throwOnError( value.size() == 2 );
    throwOnError( value.find( CameraHotzoneValueFields::Name ) != value.cend() );

    auto detectedIt = value.find( CameraHotzoneValueFields::Detected );
    throwOnError( detectedIt != value.cend() && detectedIt->is_object() );

    if( !detectedIt->empty() ) {
        auto& detected = *detectedIt;
        for( auto it = detected.begin(); it != detected.end(); ++it ) {
            throwOnError( isKnownType( it.key() ) && it->is_array() );
        }
    }

    return std::unique_ptr< CameraHotzoneValue >{ new CameraHotzoneValue{ value } };
}

std::unique_ptr< CameraHotzoneValue > CameraHotzoneValue::createDefaultValue() {
    namespace Fields = Resources::CameraHotzoneValueFields;
    auto defaultValue = JSON( Fields::Name << "" << Fields::Detected << JSON_EMPTY_OBJ );
    return std::unique_ptr< CameraHotzoneValue >{ new CameraHotzoneValue{ std::move( defaultValue ) } };
}

CameraHotzoneValue::~CameraHotzoneValue() = default;

Json CameraHotzoneValue::toJson() const {
    return m_value;
}

void CameraHotzoneValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void CameraHotzoneValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool CameraHotzoneValue::isKnownType( const std::string& type ) {
    using namespace Resources::CameraHotzoneValueFields;
    static const std::unordered_set< std::string > knownTypes{ Objects, Persons, QRCodes };
    return knownTypes.find( type ) != knownTypes.cend();
}

bool CameraHotzoneValue::isEqual( const Value& other ) const {
    const auto& object = valueCast< CameraHotzoneValue >( other );
    return m_value.dump() == object.m_value.dump();
}

void CameraHotzoneValue::set( const Value& other ) {
    const auto& object = valueCast< CameraHotzoneValue >( other );
    m_value = object.m_value;
}

CameraHotzoneValue::CameraHotzoneValue( Json value )
: Value( ValueType::Enum::CameraHotzone )
, m_value( std::move( value ) ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
