#include "house_mode_alarm_delay_update.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_52_house_mode_alarm_delay_update {

namespace DbNames {
constexpr char HUBSettings[] = "db.firmware.cored.settings";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char entryDelay[] = "entryDelay";
const char entryDelayNormal[] = "normal";
const char alarmDelay[] = "alarmDelay";
};  // namespace Fields

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    LOGGER_DEBUG_RAW( "House Mode Alarm Delay Update::migrate called" );

    auto houseModeSetting = m_dbClient.findOne( DbNames::HUBSettings,
                                                JSON( "_id"
                                                      << "houseMode" ),
                                                nullptr );
    Json entryDelayJson;

    json_get( houseModeSetting, Fields::entryDelay, entryDelayJson );
    auto alarmDelay = json_get_int( houseModeSetting, Fields::alarmDelay );
    auto normalDelay = json_get_int( entryDelayJson, Fields::entryDelayNormal );

    if( alarmDelay == 0 ) {
        alarmDelay = normalDelay;
    }
    else {
        normalDelay = alarmDelay;
    }
    houseModeSetting[ Fields::entryDelay ][ Fields::entryDelayNormal ] = normalDelay;
    houseModeSetting[ Fields::alarmDelay ] = alarmDelay;

    const std::string id = json_get_str( houseModeSetting, Fields::id );
    m_dbClient.update( DbNames::HUBSettings, JSON( Fields::id << id ), houseModeSetting );

    return true;
}

}  // namespace v1_52_house_mode_alarm_delay_update
}  // namespace MigrationTool
}  // namespace HubPlatform
