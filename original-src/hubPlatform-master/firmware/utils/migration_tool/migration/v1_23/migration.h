#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_23 {

/*

CHANGELOG:
    - Rename 'Cameras.cameras' database to 'db.addons.cameras.streams'
    - Create ssh tunnels database for existing camera_streaming items

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_23
}  // namespace MigrationTool
}  // namespace HubPlatform
