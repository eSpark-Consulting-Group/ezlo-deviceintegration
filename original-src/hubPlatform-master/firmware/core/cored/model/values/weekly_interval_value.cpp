#include "cored/model/values/weekly_interval_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< WeeklyIntervalValue > WeeklyIntervalValue::createValue( const Json& value ) {
    using namespace JsonCustomGetters;
    using namespace Resources;

    struct {
        WeekDays weekDays;
        std::string startTime;
        std::string stopTime;
    } interval;

    static const auto getter =
        makeStructGetter( makeMemberMapping( &decltype( interval )::weekDays, WeeklyIntervalFields::WeekDays ),
                          makeMemberMapping( &decltype( interval )::startTime, WeeklyIntervalFields::StartTime ),
                          makeMemberMapping( &decltype( interval )::stopTime, WeeklyIntervalFields::StopTime ) );

    if( !getter( value, interval ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::WeeklyInterval };
    }

    // TODO: may be sort weekDays and ensure unique

    return std::unique_ptr< WeeklyIntervalValue >{ new WeeklyIntervalValue{
        std::move( interval.weekDays ), std::move( interval.startTime ), std::move( interval.stopTime ) } };
}

std::unique_ptr< WeeklyIntervalValue > WeeklyIntervalValue::createDefaultValue() {
    return std::unique_ptr< WeeklyIntervalValue >{ new WeeklyIntervalValue{ {}, {}, {} } };
}

WeeklyIntervalValue::~WeeklyIntervalValue() = default;

Json WeeklyIntervalValue::toJson() const {
    Json result = Json::object();

    using namespace Resources;

    result[ WeeklyIntervalFields::WeekDays ] = m_weekDays;
    result[ WeeklyIntervalFields::StartTime ] = m_startTime;
    result[ WeeklyIntervalFields::StopTime ] = m_stopTime;

    return result;
}

void WeeklyIntervalValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void WeeklyIntervalValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool WeeklyIntervalValue::isEqual( const Value& other ) const {
    const auto& weeklyIntervalValue = valueCast< WeeklyIntervalValue >( other );
    return m_weekDays == weeklyIntervalValue.m_weekDays && m_startTime == weeklyIntervalValue.m_startTime &&
           m_stopTime == weeklyIntervalValue.m_stopTime;
}

void WeeklyIntervalValue::set( const Value& other ) {
    const auto& weeklyIntervalValue = valueCast< WeeklyIntervalValue >( other );
    m_weekDays = weeklyIntervalValue.m_weekDays, m_startTime = weeklyIntervalValue.m_startTime;
    m_stopTime = weeklyIntervalValue.m_stopTime;
}

WeeklyIntervalValue::WeeklyIntervalValue( WeekDays&& weekDays, std::string&& startTime, std::string&& stopTime )
: Value( ValueType::Enum::WeeklyInterval )
, m_weekDays( std::move( weekDays ) )
, m_startTime( std::move( startTime ) )
, m_stopTime( std::move( stopTime ) ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
