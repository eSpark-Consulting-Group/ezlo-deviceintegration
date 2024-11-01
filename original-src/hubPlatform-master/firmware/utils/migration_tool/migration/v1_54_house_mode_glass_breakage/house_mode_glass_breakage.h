#pragma once
#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_54_house_mode_glass_breakage_sensor {

namespace DbNames {
const char HaHouseModesDBName[] = "db.firmware.cored.houseModes";
const char HUBSettings[] = "db.firmware.cored.settings";
const char HaDevicesDBName[] = "db.firmware.cored.devices";
}  // namespace DbNames

namespace Fields {
const char Id[] = "_id";
const char DisarmedDefault[] = "disarmedDefault";
const char Category[] = "category";
const char Subcategory[] = "subcategory";
const char SecuritySensor[] = "security_sensor";
const char Glass[] = "glass";
const char Armed[] = "armed";
const char houseModeEntry[] = "houseMode";
};  // namespace Fields

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_54_house_mode_glass_breakage_sensor
}  // namespace MigrationTool
}  // namespace HubPlatform
