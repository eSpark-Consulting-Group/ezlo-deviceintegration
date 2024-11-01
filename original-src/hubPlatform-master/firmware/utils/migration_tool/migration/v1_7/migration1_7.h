#pragma once

#include "migration_tool/migration/migration.h"
#include "provision_data_utils/provision_data.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_7 {

/*

CHANGELOG:
    - Encrypt wifi encryption keys and user codes in database

*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient, const ProvisionDataUtils::ProvisionData& provisionData );
    bool migrate() override;

private:
    bool migrateWiFiPasswords();
    bool migrateUserCodes();

private:
    IDbClient& m_dbClient;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
};

}  // namespace v1_7
}  // namespace MigrationTool
}  // namespace HubPlatform
