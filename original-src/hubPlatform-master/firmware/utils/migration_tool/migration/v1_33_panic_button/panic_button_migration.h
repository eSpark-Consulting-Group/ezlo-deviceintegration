#pragma once

#pragma once

#include "migration_tool/migration/migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

/*

CHANGELOG:
    - To every mode/armedCategories add new record: { "category": "scene_controller", "subcategories": [ "panic" ] }

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_33_panic {

class PanicButtonMigration : public HubPlatform::MigrationTool::Migration {
public:
    explicit PanicButtonMigration( IDbClient& dbClient );
    bool migrate() override;
    int recordsModified() const;

private:
    IDbClient& m_dbClient;
    int m_recordsModified{ 0 };
};

}  // namespace v1_33_panic
}  // namespace MigrationTool
}  // namespace HubPlatform