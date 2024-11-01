#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_48 {

/*

CHANGELOG:
    - Rename 'db.firmware.scenes_manager.device_groups' database to 'db.firmware.cored.device_groups'
    - Rename 'db.firmware.scenes_manager.item_groups' database to 'db.firmware.cored`.item_groups'
    - Fix: Rename 'db.firmware.cored`.item_groups' to 'db.firmware.cored.item_groups'

*/

namespace DbNames {
extern const char HaScenesDeviceGroupsDBName[];
extern const char HaScenesItemGroupsDBName[];
extern const char HaDeviceGroupsDBName[];
extern const char HaItemGroupsDBName[];
}  // namespace DbNames

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

protected:
    void renameTable( const std::string& oldTable, const std::string& newTable );
    bool copyTable( const std::string& oldTable, const std::string& newTable );

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_48
}  // namespace MigrationTool
}  // namespace HubPlatform
