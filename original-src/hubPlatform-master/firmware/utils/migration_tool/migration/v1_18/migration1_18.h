#pragma once

#include "migration_tool/migration/migration.h"
#include "provision_data_utils/provision_data.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_18 {

/*

CHANGELOG:
    - All zigbee devices manufacturer_id and model_id fields should not be empty

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    bool migrateField( const std::string& fieldName );

    IDbClient& m_dbClient;
};

}  // namespace v1_18
}  // namespace MigrationTool
}  // namespace HubPlatform
