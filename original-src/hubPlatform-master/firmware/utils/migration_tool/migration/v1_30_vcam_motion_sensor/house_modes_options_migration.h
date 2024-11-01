#pragma once

#include "migration_tool/migration/migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "vistacam_motion_sensor_migration.h"

/*

CHANGELOG:
    - Fetch vistaCam motion sensor device identifiers and add them all to Home and Night house modes:
        houseMode[id="1" | name="3"].disarmedDevices[].
    - Fetch siren device identifiers and add them all to houseMode[id = "1" | "2" | "3" | "4"].alarmsOffDevices[].
    - Do these changes only for the house modes with disarmedDefault = true flag set.

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_30_houseModesOptions {

class HouseModesOptionsMigration : public HubPlatform::MigrationTool::Migration {
public:
    explicit HouseModesOptionsMigration( IDbClient& dbClient );
    bool migrate() override;
    int devicesAdded() const;

private:
    IDbClient& m_dbClient;
    int m_devicesAdded{ 0 };
};

}  // namespace v1_30_houseModesOptions
}  // namespace MigrationTool
}  // namespace HubPlatform