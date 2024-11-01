#include "migration_tool/data_resetter.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace MigrationTool {

DataResetter::DataResetter( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool DataResetter::resetData( bool soft ) {
    Log::info( "Resetting data..." );

    const std::set< std::string > essentialCollections = {
        WifiSettingsDBName, DbVersionsDBName, BatteryDefaultDBName, ZigbeeFlasherDBName
    };

    auto collectionsToRemove =
        m_dbClient.getAllNames( soft ? essentialCollections : std::set< std::string >(), false /*onlyWithChanges*/ );
    for( auto& collectionName : collectionsToRemove ) {
        Log::debug( "Resetting table: " + collectionName );
        m_dbClient.drop( collectionName );
    }

    Log::debug( "Removing local mode certificates" );
    filesystem::deleteFolder( Environment::getCertificateFolder() + "/local" );

    Log::info( "Reset done successfully!" );
    return true;
}

}  // namespace MigrationTool
}  // namespace HubPlatform
