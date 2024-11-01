#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_25 {

/*
CHANGELOG:
    - Drop database table 'db.firmware.networkd.cloud.ssh.tunnels'
*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_25
}  // namespace MigrationTool
}  // namespace HubPlatform