#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_20 {

/*

CHANGELOG:
    - Remove db.firmware.networkd.modemSettings.backup table

*/

constexpr char m_dataTableName[] = "db.firmware.networkd.modemSettings.backup";

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_20
}  // namespace MigrationTool
}  // namespace HubPlatform
