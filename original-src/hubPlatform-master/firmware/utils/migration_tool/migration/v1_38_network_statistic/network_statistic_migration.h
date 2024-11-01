#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_38_network_statistic {

/*

CHANGELOG:
    - Initialize a network statistic settings table by setting a 'storageSize' field to a default value - 30 (days)

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_38_network_statistic
}  // namespace MigrationTool
}  // namespace HubPlatform
