#pragma once

#include "migration_tool/migration/migration.h"
#include "json_utils/json_utils.h"
#include <set>
#include <map>

class IDbClient;

namespace HubPlatform {
class ConfigReader;
}

namespace HubPlatform {
namespace MigrationTool {
namespace v1_29_sirenHouseModes {

/*

CHANGELOG:
    - Fetch all siren devices and add their ids to alarmsOffDevices[] table.

*/

class SirenHouseModesMigration : public MigrationTool::Migration {
public:
    explicit SirenHouseModesMigration( IDbClient& dbClient, ConfigReader* configReader = nullptr );
    bool migrate() override;
    int sirensAdded() const;

private:
    using AlarmsOffDeviceIds = std::set< Json >;
    using HouseModeId = std::string;
    using HouseModeOffDevices = std::map< HouseModeId, AlarmsOffDeviceIds >;

private:
    std::vector< Json > fetchAllSirens() const;
    std::vector< Json > fetchHouseModes() const;

    void updateHouseModeEntry( const std::string& houseModeId, const JsonArray& offDevicesArray );
    void storageFillDeviceIds( const std::string& houseModeId, const JsonArray& deviceIds );
    void buildStorage( const std::vector< Json >& houseModes );
    void storageMergeSirenIds( const std::vector< Json >& sirenIds, std::vector< Json >& houseModes );
    void saveToDb( std::vector< Json >& houseModes );

    bool migrateAlarmsOffDevices();
    bool migrateCoredSettingsAlarmDelay();

private:
    IDbClient& m_dbClient;
    int m_sirensAdded{ 0 };
    HouseModeOffDevices m_storage;
    ConfigReader* m_configReaderPtr{ nullptr };
};

}  // namespace v1_29_sirenHouseModes
}  // namespace MigrationTool
}  // namespace HubPlatform
