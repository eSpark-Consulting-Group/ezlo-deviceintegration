#include "networkd/remote_interface/network_statistics_settings_manager.h"

#include "database_client/fake_db/fake_db_client.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Network {
namespace UT {

/*

TESTPLAN

DONE        1.1 Check setting is set
DONE        1.2 Check setting is partially updated

*/

TEST( StatisticsSettingsTestSuite, checkSettingIsSet ) /*1.1*/ {
    HubPlatform::UT::FakeDbClient dbClient;
    StatisticsSettingsManager settingsManager( dbClient );

    StatisticsSettingsManager::StatisticsSettings setting;
    setting.m_storageSize = 10;

    dbClient.insert( NetworkStatisticSettingsDBName,
                     JSON( db::ID_FIELD_NAME << "settings"
                                             << "storageSize" << 30 ),
                     false );

    settingsManager.setSetting( setting );

    EXPECT_EQ( dbClient.findOne( NetworkStatisticSettingsDBName, JSON( db::ID_FIELD_NAME << "settings" ), nullptr ),
               JSON( db::ID_FIELD_NAME << "settings"
                                       << "storageSize" << 10 ) );

    EXPECT_EQ( settingsManager.getSetting().m_storageSize, 10 );
}

TEST( StatisticsSettingsTestSuite, checkSettingIsPartiallyUpdated ) /*1.2*/ {
    HubPlatform::UT::FakeDbClient dbClient;
    StatisticsSettingsManager settingsManager( dbClient );

    StatisticsSettingsManager::StatisticsSettings setting;
    setting.m_storageSize = 10;

    dbClient.insert( NetworkStatisticSettingsDBName,
                     JSON( db::ID_FIELD_NAME << "settings"
                                             << "storageSize" << 30 << "someSetting"
                                             << "a" ),
                     false );

    settingsManager.setSetting( setting );

    EXPECT_EQ( dbClient.findOne( NetworkStatisticSettingsDBName, JSON( db::ID_FIELD_NAME << "settings" ), nullptr ),
               JSON( db::ID_FIELD_NAME << "settings"
                                       << "storageSize" << 10 << "someSetting"
                                       << "a" ) );
}

}  // namespace UT
}  // namespace Network
}  // namespace HubPlatform
