#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_14 {

/*

CHANGELOG:
    - Remove api field in gateway DB entries

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_14
}  // namespace MigrationTool
}  // namespace HubPlatform
