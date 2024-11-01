#include "cored/model/values/daily_interval_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< DailyIntervalValue > DailyIntervalValue::createValue( const Json& value ) {
    using namespace JsonCustomGetters;
    using namespace Resources::DailyIntervalFields;

    struct {
        std::string startDateTime;
        std::string stopDateTime;
    } interval;

    static const auto getter =
        makeStructGetter( makeMemberMapping( &decltype( interval )::startDateTime, StartDateTime ),
                          makeMemberMapping( &decltype( interval )::stopDateTime, StopDateTime ) );

    if( !getter( value, interval ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::DailyInterval };
    }

    return std::unique_ptr< DailyIntervalValue >{ new DailyIntervalValue{ std::move( interval.startDateTime ),
                                                                          std::move( interval.stopDateTime ) } };
}

std::unique_ptr< DailyIntervalValue > DailyIntervalValue::createDefaultValue() {
    return std::unique_ptr< DailyIntervalValue >{ new DailyIntervalValue{ {}, {} } };
}

DailyIntervalValue::~DailyIntervalValue() = default;

Json DailyIntervalValue::toJson() const {
    Json result = Json::object();

    result[ Resources::DailyIntervalFields::StartDateTime ] = m_startDateTime;
    result[ Resources::DailyIntervalFields::StopDateTime ] = m_stopDateTime;

    return result;
}

void DailyIntervalValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void DailyIntervalValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool DailyIntervalValue::isEqual( const Value& other ) const {
    const auto& dailyIntervalValue = valueCast< DailyIntervalValue >( other );
    return m_startDateTime == dailyIntervalValue.m_startDateTime && m_stopDateTime == dailyIntervalValue.m_stopDateTime;
}

void DailyIntervalValue::set( const Value& other ) {
    const auto& dailyIntervalValue = valueCast< DailyIntervalValue >( other );
    m_startDateTime = dailyIntervalValue.m_startDateTime;
    m_stopDateTime = dailyIntervalValue.m_stopDateTime;
}

DailyIntervalValue::DailyIntervalValue( std::string&& startDateTime, std::string&& stopDateTime )
: Value( ValueType::Enum::DailyInterval )
, m_startDateTime( std::move( startDateTime ) )
, m_stopDateTime( std::move( stopDateTime ) ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
