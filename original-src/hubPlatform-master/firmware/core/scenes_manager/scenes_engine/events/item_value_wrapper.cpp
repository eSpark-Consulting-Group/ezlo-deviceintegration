#include "scenes_manager/scenes_engine/events/item_value_wrapper.h"
#include "scenes_manager/scenes_engine/events/comparable_value.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"
#include "scenes_manager/modify_scene/resources.h"

#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/array_value.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"

#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

#include <algorithm>

namespace SceneEngine {

namespace Resources {
using namespace HubPlatform::CoreD::Model;
}  // namespace Resources

namespace WrapperUtils {

enum class CompareOperator { Equal, Less };

template < typename T >
bool compare( const T& left, const T& right, CompareOperator compareOperator ) {
    switch( compareOperator ) {
    case CompareOperator::Equal:
        return left == right;
    case CompareOperator::Less:
        return left < right;
    }
    throw EventException( "Wrapper compare operation is not defined" );
}

bool isScalableType( const Resources::ValueType& type ) {
    return type.getScales().size() > 0;
}

bool compareCoreValues( const Resources::Value& left, const Resources::Value& right, CompareOperator compareOperator ) {
    const Resources::ValueType& leftValueType = left.getType();
    const Resources::ValueType& rightValueType = right.getType();

    try {
        if( leftValueType == rightValueType ) {
            return compare( left, right, compareOperator );
        }
        else if( WrapperUtils::isScalableType( leftValueType ) && WrapperUtils::isScalableType( rightValueType ) ) {
            throw EventException( "Cannot compare values: scalable values have incomparable types" );
        }
        else if( ComparableValue::isNumericType( leftValueType ) && ComparableValue::isNumericType( rightValueType ) ) {
            return compare( ComparableValue( left ), ComparableValue( right ), compareOperator );
        }
        else if( ComparableValue::isStringType( leftValueType ) && ComparableValue::isStringType( rightValueType ) ) {
            return compare( ComparableValue( left ), ComparableValue( right ), compareOperator );
        }
        throw EventException( "Cannot compare values: value types mismatch" );
    }
    catch( const EventException& e ) {
        throw;
    }
    catch( const std::exception& e ) {
        throw EventException( "Compare core values failed: " + std::string( e.what() ) );
    }
}

bool compareNonCoreValues( const ItemValueWrapper& left,
                           const ItemValueWrapper& right,
                           CompareOperator compareOperator ) {
    // Compare whatever it is
    return compare( left.getPayload(), right.getPayload(), compareOperator );
}

}  // namespace WrapperUtils

namespace PayloadUtils {
// NOTE: Set of functions to parse/convert legacy payload formats.
// These functions have to deal with plethora of incomplete/unknown/non-typed payload formats.

using Payload = std::pair< Json, ItemValueWrapper::PayloadType >;

const std::map< std::string, Resources::ValueType::Enum >& scaleToTypeMap() {
    using Enum = Resources::ValueType::Enum;

    static std::map< std::string, Enum > map = [] {
        std::map< std::string, Enum > map;
        const int end = static_cast< int >( Enum::SimpleTypesCount );
        for( auto i = 0; i < end; ++i ) {
            Enum type = static_cast< Enum >( i );
            Resources::ValueType t( type );
            const auto& scales = t.getScales();
            for( auto& scale : scales )
                map.emplace( scale, type );
        }

        // NOTE: percent does not have units of measurement and its use is ambiguous
        auto it = map.find( "percent" );
        if( it != map.cend() )
            map.erase( it );
        return map;
    }();

    return map;
}

Resources::ValueType::Enum typeFromScale( const std::string& scale ) {
    auto& map = scaleToTypeMap();
    auto it = map.find( scale );
    if( it == map.cend() )
        throw EventException( "Cannot find type for scale: " + scale );
    return it->second;
}

ItemValueWrapper::PayloadType guessPayloadType( const Json& payload ) {
    auto type = json_get_str( payload, Fields::TypeField );
    if( !type.empty() ) {
        auto valueType = Resources::ValueType::fromString( type );
        if( valueType.isValid() )
            return ItemValueWrapper::PayloadType::CoreType;
    }

    return ItemValueWrapper::PayloadType::UnknownType;
}

std::string getJsonValueType( const JsonObject& json ) {
    if( json.is_boolean() )
        return Types::BoolType;
    else if( json.is_number_integer() )
        return Types::IntType;
    else if( json.is_number_float() )
        return Types::FloatType;
    else if( json.is_string() )
        return Types::StringType;
    return "";
}

// Default CoreD item format: { "type":"<type>", "value": <value> }
Payload parseItemFormat( Json payload ) {
    auto type = json_get_str( payload, Fields::TypeField );

    // Check for deprecated scenes format: float type used instead of scalable types
    if( type == Types::FloatType ) {
        auto& value = payload.at( Fields::ValueField );
        auto scale = json_get_str( value, Fields::ScaleField );
        if( !scale.empty() ) {
            auto valueType = typeFromScale( scale );
            payload[ Fields::TypeField ] = Resources::ValueType( valueType ).toString();
        }
    }
    auto payloadType = guessPayloadType( payload );
    return Payload( std::move( payload ), payloadType );
}

// Deprecated broadcast format: { "valueType":"<type>", "value": <value> }
Payload parseBroadcastFormat( Json payload ) {
    payload[ Fields::TypeField ] = payload[ "valueType" ];
    payload.erase( "valueType" );
    auto payloadType = guessPayloadType( payload );
    return Payload( std::move( payload ), payloadType );
}

// Deprecated scalable format: { "type":"<type>", "value": <value>, "scale": "<scale>" }
Payload parseScalableFormat( Json payload ) {
    auto& value = payload.at( Fields::ValueField );
    auto scale = json_get_str( payload, Fields::ScaleField );
    auto type = json_get_str( payload, Fields::TypeField );

    if( type.empty() || type == Types::FloatType ) {
        auto valueType = typeFromScale( scale );
        type = Resources::ValueType( valueType ).toString();
    }

    auto newPayload = JSON( Fields::TypeField << type );
    newPayload[ Fields::ValueField ] = JSON( Fields::ValueField << std::move( value ) << Fields::ScaleField << scale );
    auto payloadType = guessPayloadType( newPayload );
    return Payload( std::move( newPayload ), payloadType );
}

// Deprecated format: { "value": <object> }
Payload parseObjectValueFormat( Json payload ) {
    auto& value = payload.at( Fields::ValueField );

    // Check if scalable: { "value": { "value": <value>, "scale": "<scale>"} }
    auto scale = json_get_str( value, Fields::ScaleField );
    if( scale.empty() ) {
        LOGGER_WARN_RAW( cc::warn( "Cannot determine type for non-scalable compound value" ) );
        return Payload( std::move( payload ), ItemValueWrapper::PayloadType::UnknownType );
    }

    auto valueType = typeFromScale( scale );
    auto type = Resources::ValueType( valueType ).toString();

    auto newPayload = JSON( Fields::TypeField << type << Fields::ValueField << value );
    auto payloadType = guessPayloadType( newPayload );
    return Payload( std::move( newPayload ), payloadType );
}

// Deprecated format: { "value": <non-object> }
Payload parseNonObjectValueFormat( Json payload ) {
    auto& value = payload.at( Fields::ValueField );
    auto type = getJsonValueType( value );
    if( type.empty() ) {
        LOGGER_WARN_RAW( cc::warn( "Cannot determine non-object value type" ) );
        return Payload( std::move( payload ), ItemValueWrapper::PayloadType::UnknownType );
    }
    auto newPayload = JSON( Fields::TypeField << type << Fields::ValueField << value );
    auto payloadType = guessPayloadType( newPayload );
    return Payload( std::move( newPayload ), payloadType );
}

// Deprecated format: Json( <value> )
Payload parsePlainValueFormat( Json payload ) {
    auto type = getJsonValueType( payload );
    if( type.empty() ) {
        LOGGER_WARN_RAW( cc::warn( "Cannot determine item value type: " ) + payload.dump() );
        return Payload( std::move( payload ), ItemValueWrapper::PayloadType::UnknownType );
    }
    auto newPayload = JSON( Fields::TypeField << type << Fields::ValueField << payload );
    auto payloadType = guessPayloadType( newPayload );
    return Payload( std::move( newPayload ), payloadType );
}

// NOTE: This function tries to convert all legacy payload formats to standard payload format
// { "type":"<type>", "value": <value> }
Payload convertPayload( JsonObject payload ) {
    const bool valuePresent = payload.count( Fields::ValueField );
    const bool typePresent = payload.count( Fields::TypeField );
    const bool scalePresent = payload.count( Fields::ScaleField );

    // Default CoreD item format: { "type":"<type>", "value": <value> }
    if( valuePresent && typePresent && !scalePresent ) {
        return parseItemFormat( std::move( payload ) );
    }
    // Deprecated broadcast format: { "valueType":"<type>", "value": <value> }
    else if( payload.count( "valueType" ) && valuePresent ) {
        return parseBroadcastFormat( std::move( payload ) );
    }
    // Deprecated scalable format: { "type":"<type>", "value": <value>, "scale": "<scale>" }
    else if( valuePresent && scalePresent ) {
        return parseScalableFormat( std::move( payload ) );
    }
    // Deprecated format: { "value": <value> }
    else if( valuePresent && !scalePresent && !typePresent ) {
        auto& value = payload.at( Fields::ValueField );
        if( value.is_object() )
            return parseObjectValueFormat( std::move( payload ) );
        else
            return parseNonObjectValueFormat( std::move( payload ) );
    }
    // Deprecated format: Json( <value> )
    else if( !valuePresent && !payload.is_object() ) {
        return parsePlainValueFormat( std::move( payload ) );
    }
    else {
        LOGGER_WARN_RAW( cc::warn( "Unknown payload format: " + cc::note( payload.dump() ) ) );
    }
    return Payload( std::move( payload ), ItemValueWrapper::PayloadType::UnknownType );
}

}  // namespace PayloadUtils

ItemValueWrapper::ItemValueWrapper() = default;
ItemValueWrapper::~ItemValueWrapper() = default;

using namespace HubPlatform::CoreD::Model::Resources;

ItemValueWrapper::ItemValueWrapper( JsonObject payload ) {
    setPayload( std::move( payload ) );
}

ItemValueWrapper ItemValueWrapper::fromValueAndType( JsonObject value, std::string valueType ) {
    return ItemValueWrapper(
        { { ItemFields::ValueFieldName, std::move( value ) }, { ItemFields::TypeFieldName, std::move( valueType ) } } );
}

ItemValueWrapper::ItemValueWrapper( const ItemValueWrapper& other ) {
    *this = other;
}

ItemValueWrapper& ItemValueWrapper::operator=( const ItemValueWrapper& other ) {
    if( this != &other ) {
        m_payload = other.m_payload;
        m_payloadType = other.m_payloadType;

        if( other.m_payloadType == PayloadType::CoreType ) {
            if( other.isValid() )
                parseCoreValue();
            else
                m_coreValueOrError = other.getCoreValueParseError();
        }
    }

    return *this;
}

ItemValueWrapper::ItemValueWrapper( ItemValueWrapper&& other ) = default;
ItemValueWrapper& ItemValueWrapper::operator=( ItemValueWrapper&& other ) = default;

const Json& ItemValueWrapper::getValue() const {
    auto found = m_payload.find( Fields::ValueField );
    if( found != m_payload.end() ) {
        if( !getScale().empty() ) {
            if( found->count( Fields::ValueField ) && found->count( Fields::ScaleField ) ) {
                return ( *found )[ Fields::ValueField ];
            }
        }
        return *found;
    }

    static Json empty;
    return empty;
}

const Json& ItemValueWrapper::getRawValue() const {
    auto found = m_payload.find( Fields::ValueField );
    if( found != m_payload.end() )
        return *found;
    static Json empty;
    return empty;
}

std::string ItemValueWrapper::getScale() const {
    auto valueFound = m_payload.find( Fields::ValueField );
    if( valueFound != m_payload.end() ) {
        auto scaleFound = valueFound->find( Fields::ScaleField );
        if( scaleFound != valueFound->end() )
            return scaleFound->get< std::string >();
    }
    return std::string{};
}

void ItemValueWrapper::setPayload( JsonObject payload ) {
    std::tie( m_payload, m_payloadType ) = PayloadUtils::convertPayload( std::move( payload ) );
    if( !m_payload.empty() && m_payloadType == PayloadType::CoreType ) {
        parseCoreValue();
    }
}

void ItemValueWrapper::setValueType( const std::string& valueType ) {
    m_payload[ ItemFields::TypeFieldName ] = valueType;
}

/** Returns the whole json of an item */
const Json& ItemValueWrapper::getPayload() const {
    return m_payload;
}

bool ItemValueWrapper::empty() const {
    return m_payload.empty();
}

std::string ItemValueWrapper::getValueType() const {
    return json_get_str( m_payload, ItemFields::TypeFieldName );
}

bool ItemValueWrapper::isArray() const {
    const CoreValue* coreValue = getCoreValue();
    return coreValue && coreValue->getType().isArray();
}

std::vector< ItemValueWrapper > ItemValueWrapper::parseArrayValues() const {
    using namespace HubPlatform::CoreD::Model;

    if( !isArray() )
        throw EventException( "Trying to parse array values, while the value is not an array, actual type: " +
                              getValueType() );

    std::vector< ItemValueWrapper > result;
    try {
        const CoreValue* coreValue = getCoreValue();
        if( coreValue ) {
            const auto& array = valueCast< ArrayValue >( *coreValue );
            const std::string arrayValuesTypeStr = array.getValuesType().toString();

            array.forEachValue( [&result, &arrayValuesTypeStr]( const Value& value ) {
                Json payload = { { "value", value.toJson() }, { "type", arrayValuesTypeStr } };
                result.emplace_back( std::move( payload ) );
            } );
        }
    }
    catch( const CoreDModelError& ex ) {
        LOGGER_ERROR( cc::error(
            "Parsing of element of already parsed valid CoreD's array failed, there should be an error in logic" ) );
        throw EventException( "Parsing of array values failed: " + std::string( ex.what() ) );
    }

    return result;
}

bool ItemValueWrapper::isValid() const noexcept {
    if( m_payload.empty() )
        return false;

    // NOTE: we can't validate arbitrary json format so we assume it is correct
    const bool hasValidCoreValue = m_payloadType == PayloadType::CoreType && getCoreValue() != nullptr;
    return hasValidCoreValue || m_payloadType == PayloadType::UnknownType;
}

std::string ItemValueWrapper::getInvalidityReason() const noexcept {
    if( m_payload.empty() ) {
        return "Empty payload";
    }

    if( m_payloadType == PayloadType::CoreType ) {
        return getCoreValueParseError();
    }

    return std::string();
}

const ItemValueWrapper::CoreValue* ItemValueWrapper::getCoreValue() const noexcept {
    if( mpark::holds_alternative< CoreValueUPtr >( m_coreValueOrError ) ) {
        return mpark::get< CoreValueUPtr >( m_coreValueOrError ).get();
    }

    return nullptr;
}

std::string ItemValueWrapper::getCoreValueParseError() const noexcept {
    if( mpark::holds_alternative< std::string >( m_coreValueOrError ) ) {
        return mpark::get< std::string >( m_coreValueOrError );
    }

    return {};
}

bool ItemValueWrapper::isComparableWith( const ItemValueWrapper& other ) const {
    using HubPlatform::CoreD::Model::ValueType;
    if( m_payloadType == PayloadType::CoreType && other.m_payloadType == PayloadType::CoreType ) {
        const CoreValue* coreValue = getCoreValue();
        const CoreValue* coreValueOther = other.getCoreValue();
        if( coreValue && coreValueOther ) {
            const ValueType& valueType = coreValue->getType();
            const ValueType& valueTypeOther = coreValueOther->getType();
            if( valueType == valueTypeOther )
                return true;
            if( WrapperUtils::isScalableType( valueType ) && WrapperUtils::isScalableType( valueTypeOther ) )
                return false;
            const bool bothStringType =
                ComparableValue::isStringType( valueType ) && ComparableValue::isStringType( valueTypeOther );
            const bool bothNumericType = !bothStringType && ComparableValue::isNumericType( valueType ) &&
                                         ComparableValue::isNumericType( valueTypeOther );
            return bothStringType || bothNumericType;
        }
    }

    return m_payloadType == PayloadType::UnknownType && other.m_payloadType == PayloadType::UnknownType;
}

bool ItemValueWrapper::operator==( const ItemValueWrapper& other ) const {
    return compareEqual( other );
}

bool ItemValueWrapper::operator!=( const ItemValueWrapper& other ) const {
    return !this->operator==( other );
}

bool ItemValueWrapper::operator<( const ItemValueWrapper& other ) const {
    return compareLess( other );
}

bool ItemValueWrapper::operator>( const ItemValueWrapper& other ) const {
    return other < *this;
}

bool ItemValueWrapper::operator<=( const ItemValueWrapper& other ) const {
    return !( other < *this );
}

bool ItemValueWrapper::operator>=( const ItemValueWrapper& other ) const {
    return !( *this < other );
}

void ItemValueWrapper::parseCoreValue() {
    using namespace HubPlatform::CoreD::Model;

    bool hasValidType = false;

    const std::string valueType = getValueType();
    if( !valueType.empty() ) {
        ValueType coreValueType = Resources::ValueType::fromString( valueType );
        if( coreValueType.isValid() ) {
            hasValidType = true;

            try {
                CoreValueUPtr coreValue = ValueFactory::createItemValue( std::move( coreValueType ), getRawValue() );
                if( coreValue )
                    m_coreValueOrError = std::move( coreValue );
                else
                    m_coreValueOrError =
                        std::string{ "CoreD value wasn't created due to unknown reason, value type: " } + valueType;
            }
            catch( const CoreDModelError& ex ) {
                m_coreValueOrError = std::string( ex.what() );
            }
        }
    }

    if( !hasValidType ) {
        m_coreValueOrError = std::string( "Valid CoreD value type was not specified in payload" );
    }
}

bool ItemValueWrapper::compareEqual( const ItemValueWrapper& other ) const {
    return compareWrapperValues( *this, other, WrapperUtils::CompareOperator::Equal );
}

bool ItemValueWrapper::compareLess( const ItemValueWrapper& other ) const {
    return compareWrapperValues( *this, other, WrapperUtils::CompareOperator::Less );
}

bool ItemValueWrapper::compareWrapperValues( const ItemValueWrapper& left,
                                             const ItemValueWrapper& right,
                                             WrapperUtils::CompareOperator compareOperator ) {
    auto compareOperatorToStr = []( WrapperUtils::CompareOperator compareOperator ) -> std::string {
        switch( compareOperator ) {
        case WrapperUtils::CompareOperator::Equal:
            return "==";
        case WrapperUtils::CompareOperator::Less:
            return "<";
        }
        throw EventException( "Wrapper operator does not have string representation" );
    };
    auto payloadTypeToStr = []( PayloadType payloadType ) -> std::string {
        return payloadType == PayloadType::CoreType ? "CoreType" : "UnknownType";
    };

    auto comparatorStr = compareOperatorToStr( compareOperator );
    LOGGER_TRACE_RAW( cc::trace( "Compare \"" + comparatorStr + "\" for : " ) + cc::j( left.getPayload() ) +
                      cc::trace( " and " ) + cc::j( right.getPayload() ) );

    if( left.m_payloadType != right.m_payloadType ) {
        throw EventException( "Cannot compare " + comparatorStr +
                              " for mixed payload types: " + payloadTypeToStr( left.m_payloadType ) + " and " +
                              payloadTypeToStr( right.m_payloadType ) );
    }

    if( left.m_payloadType == PayloadType::CoreType && right.m_payloadType == PayloadType::CoreType ) {
        if( !left.getCoreValue() )
            throw EventException( "Cannot compare values: left operand doesn't have a valid core value" );
        if( !right.getCoreValue() )
            throw EventException( "Cannot compare values: right operand doesn't have a valid core value" );

        return WrapperUtils::compareCoreValues( *left.getCoreValue(), *right.getCoreValue(), compareOperator );
    }
    else if( left.m_payloadType == PayloadType::UnknownType && right.m_payloadType == PayloadType::UnknownType ) {
        return WrapperUtils::compareNonCoreValues( left, right, compareOperator );
    }

    throw EventException( "Cannot compare " + comparatorStr + " for unknown payload types" );
}

}  // namespace SceneEngine
