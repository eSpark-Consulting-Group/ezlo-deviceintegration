#include "scenes_manager/scenes_engine/events/interval_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include <functional>
#include <unordered_map>

namespace SceneEngine {
using DurationFuncT = std::function< tiny::dispatch::duration_t( const tiny::dispatch::duration_base_t& value ) >;

DurationFuncT getDuration( const std::string& suffix ) {
    using AssocFuncMapT = std::unordered_map< std::string, DurationFuncT >;
    static const AssocFuncMapT funcMap = {
        { "s", tiny::dispatch::duration_from_seconds< const tiny::dispatch::duration_base_t > },
        { "m", tiny::dispatch::duration_from_minutes< const tiny::dispatch::duration_base_t > },
        { "h", tiny::dispatch::duration_from_hours< const tiny::dispatch::duration_base_t > },
        { "d", tiny::dispatch::duration_from_days< const tiny::dispatch::duration_base_t > }
    };

    const auto it = funcMap.find( suffix );
    if( it != funcMap.cend() ) {
        return it->second;
    }
    return nullptr;
}

IntervalEvent::IntervalEvent( const JsonObject& intervalValue ) {
    //"s" for seconds, "m" for minutes, "h" for hours and "d" for days
    if( intervalValue.is_string() ) {
        auto data = intervalValue.get< std::string >();
        std::size_t pos = std::string::npos;
        long value = std::stol( data, &pos );
        if( pos != std::string::npos ) {
            std::string suffix = data.substr( pos );
            auto func = getDuration( suffix );
            if( func ) {
                auto duration = func( static_cast< tiny::dispatch::duration_base_t >( value ) );
                m_duration = std::chrono::duration_cast< GregorianCalendar::ClockDuration >( duration );
                if( m_duration <= GregorianCalendar::ClockDuration::zero() ) {
                    throw SceneException( Errors::WRONG_FIELD, "Event interval should be positive" );
                }
            }
            else {
                // TODO: make specific error !!!
                throw SceneException( Errors::WRONG_FIELD );
            }
        }
        else {
            // TODO: make specific error !!!
            throw SceneException( Errors::WRONG_FIELD );
        }
    }
    else {
        throw SceneException( Errors::WRONG_FIELD );
    }
}

std::string IntervalEvent::getMethodName() const {
    return Model::Resources::MethodNames::isInterval;
}

DateEventBase::OptionalClock IntervalEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                         TimeZoneOffset offset ) const {
    using namespace GregorianCalendar;

    if( m_duration <= ClockDuration::zero() ) {
        LOGGER_ERROR_RAW( cc::error( "Cannot evaluate next date for isInterval, because interval is not positive " ) );
        return nullptr;
    }

    const auto diff = localNow.time_since_epoch() - m_startTime;
    const auto timeSinceStepStart = diff % m_duration;
    const auto timeToStepEnd = diff >= ClockDuration::zero() ? m_duration - timeSinceStepStart : -timeSinceStepStart;
    const auto nextDate = localNow.time_since_epoch() + timeToStepEnd;
    return std::make_unique< LocalTimePoint >( nextDate, offset, true );
}

void IntervalEvent::setStartTime( const GregorianCalendar::ClockDuration& startTime ) {
    m_startTime = startTime;
}

}  // namespace SceneEngine
