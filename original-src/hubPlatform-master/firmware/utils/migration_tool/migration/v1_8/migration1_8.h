#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_8 {

/*

CHANGELOG:
    - Remove db.firmware.uid.user collection and old certificates directory

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_8
}  // namespace MigrationTool
}  // namespace HubPlatform
