#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_6 {

/*

CHANGELOG:
    - Setting of gateway state to database if it is absent

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_6
}  // namespace MigrationTool
}  // namespace HubPlatform
