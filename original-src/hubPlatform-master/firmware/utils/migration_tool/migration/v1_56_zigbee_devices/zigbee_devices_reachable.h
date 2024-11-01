#pragma once

#include "migration_tool/migration/migration.h"
#include "provision_data_utils/provision_data.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_56_zigbee_devices {

/*

CHANGELOG:
    - Double reachable recheck timeout
    - Mark all battery-powered Centralite devices as sleepy
*/

class Migration : public MigrationTool::Migration {
public:
    Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_56_zigbee_devices
}  // namespace MigrationTool
}  // namespace HubPlatform
