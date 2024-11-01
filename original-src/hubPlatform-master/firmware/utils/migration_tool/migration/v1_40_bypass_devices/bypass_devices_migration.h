#pragma once

#include "migration_tool/migration/migration.h"

/*

CHANGELOG:
    - Housed modes extended with a new property - bypass devices.
      The migration adds needed field to the db records.

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_40_bypass_devices {

namespace DbNames {
extern const char HaHouseModesDBName[];
}  // namespace DbNames

namespace Fields {
extern const char id[];
extern const char bypassDevices[];
};  // namespace Fields

class BypassDevicesMigration : public Migration {
public:
    explicit BypassDevicesMigration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_40_bypass_devices
}  // namespace MigrationTool
}  // namespace HubPlatform
