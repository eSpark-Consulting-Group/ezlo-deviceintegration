#pragma once

#include "migration_tool/migration/migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

/*

CHANGELOG:
    - To every mode/armedCategories where category="security_sensor" add a new subcategory="heat"

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_32_overheat {

class OverheatMigration : public HubPlatform::MigrationTool::Migration {
public:
    explicit OverheatMigration( IDbClient& dbClient );
    bool migrate() override;
    int recordsModified() const;

private:
    IDbClient& m_dbClient;
    int m_recordsModified{ 0 };
};

}  // namespace v1_32_overheat
}  // namespace MigrationTool
}  // namespace HubPlatform