#include "cored/model/values/shutter_second_part.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"

#include <unordered_set>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< ShutterSecondPartValue > ShutterSecondPartValue::createValue( const Json& value ) {
    using namespace Resources::ShutterSecondPart;

    auto deviceIdIt = value.find( Fields::DeviceId );
    if( deviceIdIt == value.end() || !deviceIdIt->is_string() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ShutterSecondPart };
    }

    auto actionIt = value.find( Fields::PartFunction );
    if( actionIt == value.end() || !actionIt->is_string() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ShutterSecondPart };
    }

    static const std::unordered_set< std::string > partFunctionsSet = {
        PartFunctions::Unknown, PartFunctions::Front, PartFunctions::Back
    };
    auto function = actionIt->get< std::string >();

    if( !partFunctionsSet.count( function ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ShutterSecondPart };
    }

    return std::unique_ptr< ShutterSecondPartValue >{ new ShutterSecondPartValue{ deviceIdIt->get< std::string >(),
                                                                                  std::move( function ) } };
}

std::unique_ptr< ShutterSecondPartValue > ShutterSecondPartValue::createDefaultValue() {
    using namespace Resources::ShutterSecondPart;
    return std::unique_ptr< ShutterSecondPartValue >{ new ShutterSecondPartValue{ InvalidDeviceId,
                                                                                  PartFunctions::Unknown } };
}

ShutterSecondPartValue::ShutterSecondPartValue( std::string deviceId, std::string partFunction )
: Value{ ValueType::Enum::ShutterSecondPart }
, m_deviceId{ std::move( deviceId ) }
, m_partFunction{ std::move( partFunction ) } {}

ShutterSecondPartValue::~ShutterSecondPartValue() = default;

Json ShutterSecondPartValue::toJson() const {
    using namespace Resources::ShutterSecondPart;

    return JSON( Fields::DeviceId << m_deviceId << Fields::PartFunction << m_partFunction );
}

void ShutterSecondPartValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ShutterSecondPartValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool ShutterSecondPartValue::isEqual( const Value& other ) const {
    const auto& otherValueNameValue = valueCast< ShutterSecondPartValue >( other );
    return m_deviceId == otherValueNameValue.m_deviceId && m_partFunction == otherValueNameValue.m_partFunction;
}

void ShutterSecondPartValue::set( const Value& other ) {
    const auto& otherValueNameValue = valueCast< ShutterSecondPartValue >( other );
    m_deviceId = otherValueNameValue.m_deviceId;
    m_partFunction = otherValueNameValue.m_partFunction;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
