#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_31 {

/*

CHANGELOG:
    - Add a 'settings' field to each gateway registered in the system and set it to 0

*/

constexpr char gatewaysTableName[] = "db.firmware.cored.gateways";
constexpr char settingsFieldName[] = "settings";

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_31
}  // namespace MigrationTool
}  // namespace HubPlatform
