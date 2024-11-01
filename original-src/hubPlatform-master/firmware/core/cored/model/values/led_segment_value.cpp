#include "cored/model/values/led_segment_value.h"

#include "cored/model/exceptions.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< LedSegmentValue > LedSegmentValue::createValue( const Json& value ) {
    using namespace Resources::LedSegmentValueFields;
    using namespace JsonCustomGetters;

    auto result = createDefaultValue();

    const auto getter = makeStructGetter( makeMemberMapping( &LedSegmentValue::m_segmentName, SegmentNameFieldName ),
                                          makeMemberMapping( &LedSegmentValue::m_segmentStart, SegmentStartFieldName ),
                                          makeMemberMapping( &LedSegmentValue::m_segmentEnd, SegmentEndFieldName ) );

    if( !getter( value, *result ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::LedSegment };
    }

    return result;
}

std::unique_ptr< LedSegmentValue > LedSegmentValue::createDefaultValue() {
    return std::unique_ptr< LedSegmentValue >{ new LedSegmentValue{ {}, {}, {} } };
}

LedSegmentValue::LedSegmentValue( std::string segmentName, uint32_t segmentStart, uint32_t segmentEnd )
: Value( ValueType::Enum::LedSegment )
, m_segmentName( std::move( segmentName ) )
, m_segmentStart( segmentStart )
, m_segmentEnd( segmentEnd ) {}

LedSegmentValue::~LedSegmentValue() = default;

Json LedSegmentValue::toJson() const {
    using namespace Resources::LedSegmentValueFields;

    auto result = Json::object();

    result[ SegmentNameFieldName ] = m_segmentName;
    result[ SegmentStartFieldName ] = m_segmentStart;
    result[ SegmentEndFieldName ] = m_segmentEnd;

    return result;
}

bool LedSegmentValue::isEqual( const Value& other ) const {
    const auto& otherSegmentValue = valueCast< LedSegmentValue >( other );
    return m_segmentName == otherSegmentValue.m_segmentName && m_segmentStart == otherSegmentValue.m_segmentStart &&
           m_segmentEnd == otherSegmentValue.m_segmentEnd;
}

void LedSegmentValue::set( const Value& other ) {
    const auto& otherUserCodeValue = valueCast< LedSegmentValue >( other );
    m_segmentName = otherUserCodeValue.m_segmentName;
    m_segmentStart = otherUserCodeValue.m_segmentStart;
    m_segmentEnd = otherUserCodeValue.m_segmentEnd;
}

void LedSegmentValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void LedSegmentValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
