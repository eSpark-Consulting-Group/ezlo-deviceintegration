#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_27 {

/*

CHANGELOG:
    - Add a new field 'protect' for each house mode entry
*/

constexpr char m_dataTableName[] = "db.firmware.cored.houseModes";

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_27
}  // namespace MigrationTool
}  // namespace HubPlatform
