#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_2 {

/*

CHANGELOG:
    - Setting of default user notification list to notify all state in house modes database

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_2
}  // namespace MigrationTool
}  // namespace HubPlatform
