#include "migration_tool/migration/v1_1/migration1_1.h"

#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"

#include "database_client/idbclient.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_1 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto houseModeSettings = m_dbClient.findOne( "db.firmware.cored.settings",
                                                 JSON( "_id"
                                                       << "houseMode" ),
                                                 nullptr );
    if( !houseModeSettings.empty() ) {
        int switchToDelay = 30;
        int alarmDelay = 30;

        try {
            ConfigReader configReader;

            if( configReader.hasValue( ConfigKeys::HouseModeSwitchToDelay ) ) {
                switchToDelay = configReader.getInt( ConfigKeys::HouseModeSwitchToDelay );
            }

            if( configReader.hasValue( ConfigKeys::HouseModeAlarmDelay ) ) {
                alarmDelay = configReader.getInt( ConfigKeys::HouseModeAlarmDelay );
            }
        }
        catch( const std::exception& error ) {
            Log::warn( cc::warn( "Cannot get house mode switch delay and alarm delay from configs: " ) +
                       cc::note( error.what() ) );
        }

        m_dbClient.update( "db.firmware.cored.settings",
                           JSON( "_id"
                                 << "houseMode" ),
                           JSON( "$set" << JSON( "switchToDelay" << switchToDelay << "alarmDelay" << alarmDelay ) ) );
    }

    constexpr char HouseModesTableName[] = "db.firmware.cored.houseModes";
    const auto fieldsToReturn = JSON( "_id"
                                      << "_id" );
    auto existingHouseModes = m_dbClient.findAll( HouseModesTableName, JSON_EMPTY_OBJ, &fieldsToReturn );

    std::string extractedHouseModeId;
    for( auto& houseModeId : existingHouseModes ) {
        auto houseMode = m_dbClient.findOne( HouseModesTableName, houseModeId, nullptr );
        if( !houseMode.empty() ) {
            houseMode.erase( "switchToDelay" );

            houseMode[ "ignoreSwitchDelay" ] =
                json_get( houseMode, "_id", extractedHouseModeId ) && ( extractedHouseModeId == "1" );

            m_dbClient.remove( HouseModesTableName, houseModeId );
            m_dbClient.insert( HouseModesTableName, houseMode );
        }
    }

    return true;
}

}  // namespace v1_1
}  // namespace MigrationTool
}  // namespace HubPlatform
