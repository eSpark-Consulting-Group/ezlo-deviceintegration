
#include "common_utils/math.h"
#include "json_utils/json_utils.h"
#include <tiny/tiny_dispatch.h>
#include "jsonrpc/error_codes.h"
#include "base_utils/gregorian_calendar.h"

#include <cmath>

namespace HubPlatform {
namespace CommonUtils {

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

bool timeFormatter( const std::string& data, uint64_t& result ) {
    //"s" for seconds, "m" for minutes, "h" for hours and "d" for days
    try {
        std::size_t pos = std::string::npos;
        long value = std::stol( data, &pos );
        if( pos != std::string::npos ) {
            std::string suffix = data.substr( pos );
            auto func = getDuration( suffix );
            if( func ) {
                auto duration = func( static_cast< tiny::dispatch::duration_base_t >( value ) );
                auto res = std::chrono::duration_cast< GregorianCalendar::ClockDuration >( duration );
                if( duration <= GregorianCalendar::ClockDuration::zero() ) {
                    LOGGER_ERROR( "Time formatter error" );
                }
                result = res.count();
                return true;
            }
            else {
                LOGGER_ERROR( "Time formatter error: suffix error" );
                result = 0;
                return false;
            }
        }
        else {
            LOGGER_ERROR( "Time formatter error: value convert error" );
            return false;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Time formatter error:" + ex.what() );
        return false;
    }
}

}  // namespace CommonUtils
}  // namespace HubPlatform
