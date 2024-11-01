#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_26 {

/*

CHANGELOG:
    - Remove db.firmware.networkd.modemSettings.current table
      (due to bug this table has no useful information, so it can be removed safely)
*/

constexpr char m_dataTableName[] = "db.firmware.networkd.modemSettings.current";

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_26
}  // namespace MigrationTool
}  // namespace HubPlatform
