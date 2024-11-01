#pragma once
#pragma once

#include "migration_tool/migration/migration.h"

/*

CHANGELOG:
    - Change default "protect" feature value from "off" to "on" for e450 hub
*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_47_protect_feature_on_e450 {

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_47_protect_feature_on_e450
}  // namespace MigrationTool
}  // namespace HubPlatform
