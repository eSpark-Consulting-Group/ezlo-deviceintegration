#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_24 {

/*

CHANGELOG:
    - Setting of inclusion setting to provisioning list database if it is absent

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_24
}  // namespace MigrationTool
}  // namespace HubPlatform
