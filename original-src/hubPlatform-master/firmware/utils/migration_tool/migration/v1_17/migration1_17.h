#pragma once

#include "migration_tool/migration/migration.h"
#include "provision_data_utils/provision_data.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_17 {

/*

CHANGELOG:
    - Change Zigbee node data fields names to snake case

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    void convertFiledsNames( JsonObject& object ) const;
    std::string toSnakeCase( const std::string& name ) const;

    IDbClient& m_dbClient;
};

}  // namespace v1_17
}  // namespace MigrationTool
}  // namespace HubPlatform
