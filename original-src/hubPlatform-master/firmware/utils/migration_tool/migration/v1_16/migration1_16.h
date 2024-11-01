#pragma once

#include "migration_tool/migration/migration.h"
#include "provision_data_utils/provision_data.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_16 {

/*

CHANGELOG:
    - Add new fields to Zigbee node: lastUpdateTime, reachableStateTimeout

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_16
}  // namespace MigrationTool
}  // namespace HubPlatform
