#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_11 {

/*

CHANGELOG:
    - Remove temp.db.firmware.scenes_manager.items collection

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_11
}  // namespace MigrationTool
}  // namespace HubPlatform
