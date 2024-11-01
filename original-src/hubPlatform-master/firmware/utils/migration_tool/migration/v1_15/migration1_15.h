#pragma once

#include "migration_tool/migration/migration.h"
#include "provision_data_utils/provision_data.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_15 {

/*

CHANGELOG:
    - Encrypt peer keys in database

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient, const ProvisionDataUtils::ProvisionData& provisionData );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
};

}  // namespace v1_15
}  // namespace MigrationTool
}  // namespace HubPlatform
