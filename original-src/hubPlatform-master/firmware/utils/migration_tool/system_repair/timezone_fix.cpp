#include "migration_tool/system_repair/timezone_fix.h"

#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace MigrationTool {

namespace {
constexpr const char ZONE_NAME[] = "zonename";

}  // namespace

TimezoneFix::TimezoneFix( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

const char* TimezoneFix::getName() const {
    return "timezone fix";
}

bool TimezoneFix::shouldApply() const {
    m_expectedTimezone = getExpectedTimezone();
    if( m_expectedTimezone.empty() )
        return false;

    if( !m_systemSettings.load() )
        return false;

    auto currentTimezone = m_systemSettings.getTimezone();
    if( currentTimezone == m_expectedTimezone ) {
        Log::debug( cc::debug( "System timezone is good" ) );
        return false;
    };

    Log::debug( cc::debug( "System timezone differs from specified" ) );
    return true;
}

bool TimezoneFix::apply() {
    if( !m_systemSettings.setTimeZone( m_expectedTimezone ) )
        return false;

    if( !m_systemSettings.save() )
        return false;

    return m_systemSettings.apply();
}

std::string TimezoneFix::getExpectedTimezone() const {
    auto timezoneJson = m_dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << ZONE_NAME ), nullptr );
    auto timezoneIt = timezoneJson.find( "value" );
    if( timezoneIt == timezoneJson.cend() ) {
        Log::debug( cc::debug( "Timezone was never specified" ) );
        return {};
    }
    return timezoneIt->get< std::string >();
}

}  // namespace MigrationTool
}  // namespace HubPlatform
