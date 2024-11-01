#pragma once

#include "migration_tool/migration/migration.h"
#include "json_utils/json_fwd.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_36 {

/*

CHANGELOG:
    - Add id to each blocks in existed scenes
*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    void updateId( Json& block );

    IDbClient& m_dbClient;
};

}  // namespace v1_36
}  // namespace MigrationTool
}  // namespace HubPlatform
