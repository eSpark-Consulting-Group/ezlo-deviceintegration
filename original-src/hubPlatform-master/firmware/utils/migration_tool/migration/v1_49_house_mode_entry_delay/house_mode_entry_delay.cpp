#include <fstream>
#include "house_mode_entry_delay.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "config_reader/config_keys.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_49_house_mode_entry_delay {

namespace DbNames {
constexpr char HUBSettings[] = "db.firmware.cored.settings";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char entryDelay[] = "entryDelay";
const char entryDelayNormal[] = "normal";
const char entryDelayShort[] = "short";
const char entryDelayExtended[] = "extended";
const char entryDelayInstant[] = "instant";
};  // namespace Fields

Migration::Migration( IDbClient& dbClient, ConfigReader* configReader )
: m_dbClient( dbClient )
, m_configReaderPtr( configReader ) {}

bool Migration::migrate() {
    LOGGER_DEBUG_RAW( "House Mode Entry Delay::migrate called" );

    auto houseModeSetting = m_dbClient.findOne( DbNames::HUBSettings,
                                                JSON( "_id"
                                                      << "houseMode" ),
                                                nullptr );
    int normalDelay = 0;
    int shortDelay = 0;
    int extendedDelay = 0;
    int instantDelay = 0;

    try {
        static ConfigReader configReaderDefault;

        ConfigReader& configReader = m_configReaderPtr ? *m_configReaderPtr : configReaderDefault;
        if( configReader.hasValue( ConfigKeys::HouseModeEntryDelayNormal ) ) {
            normalDelay = configReader.getInt( ConfigKeys::HouseModeEntryDelayNormal );
        }
        if( configReader.hasValue( ConfigKeys::HouseModeEntryDelayShort ) ) {
            shortDelay = configReader.getInt( ConfigKeys::HouseModeEntryDelayShort );
        }
        if( configReader.hasValue( ConfigKeys::HouseModeEntryDelayExtended ) ) {
            extendedDelay = configReader.getInt( ConfigKeys::HouseModeEntryDelayExtended );
        }
        if( configReader.hasValue( ConfigKeys::HouseModeEntryDelayInstant ) ) {
            instantDelay = configReader.getInt( ConfigKeys::HouseModeEntryDelayInstant );
        }
    }
    catch( const std::exception& error ) {
        Log::warn( cc::warn( "Cannot get house mode entry delay data from configs: " ) + cc::note( error.what() ) );
    }

    houseModeSetting[ Fields::entryDelay ] = JSON(
        Fields::entryDelayNormal << normalDelay << Fields::entryDelayShort << shortDelay << Fields::entryDelayExtended
                                 << extendedDelay << Fields::entryDelayInstant << instantDelay );
    const std::string id = json_get_str( houseModeSetting, Fields::id );
    m_dbClient.update( DbNames::HUBSettings, JSON( Fields::id << id ), houseModeSetting );

    return true;
}

}  // namespace v1_49_house_mode_entry_delay
}  // namespace MigrationTool
}  // namespace HubPlatform
