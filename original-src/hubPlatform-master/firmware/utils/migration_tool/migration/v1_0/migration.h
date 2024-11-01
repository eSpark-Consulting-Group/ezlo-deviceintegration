#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_0 {

/*

CHANGELOG:
    - Rename tables in database according to new naming rules.

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_0
}  // namespace MigrationTool
}  // namespace HubPlatform
