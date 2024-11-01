#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_1 {

/*

CHANGELOG:
    - Added alarm delay to House Modes settings
    - Moved House Mode switch delay from all house modes to one setting
    - Added internal field for all house modes( ignoreSwitchDelay ) for ignoring delay in Home mode

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_1
}  // namespace MigrationTool
}  // namespace HubPlatform
