#include "networkd/remote_interface/network_statistics_settings_manager.h"

#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

StatisticsSettingsManager::StatisticsSettings::StatisticsSettings() = default;
StatisticsSettingsManager::StatisticsSettings::StatisticsSettings( const Json& settings ) {
    m_storageSize = json_get_int( settings, "storageSize", -1 );
}

Json StatisticsSettingsManager::StatisticsSettings::toJson() const {
    return JSON( "storageSize" << m_storageSize );
}

bool StatisticsSettingsManager::StatisticsSettings::empty() const {
    return m_storageSize == -1;
}

StatisticsSettingsManager::StatisticsSettingsManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

void StatisticsSettingsManager::setSetting( const StatisticsSettings& settings ) {
    auto jsSettings = settings.toJson();
    jsSettings[ db::ID_FIELD_NAME ] = "settings";

    m_dbClient.update( NetworkStatisticSettingsDBName, JSON_EMPTY_OBJ, JSON( "$set" << settings.toJson() ) );
}

StatisticsSettingsManager::StatisticsSettings StatisticsSettingsManager::getSetting() const {
    return StatisticsSettings(
        m_dbClient.findOne( NetworkStatisticSettingsDBName, JSON( db::ID_FIELD_NAME << "settings" ), nullptr ) );
}

}  // namespace Network
}  // namespace HubPlatform
