#pragma once
#pragma once

#include "migration_tool/migration/migration.h"

/*

CHANGELOG:
    - Housed modes extended with a new property - armed.
      The migration adds needed field to the db records.

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_45_house_mode_armed {

namespace DbNames {
extern const char HaHouseModesDBName[];
}  // namespace DbNames

namespace Fields {
extern const char id[];
extern const char armed[];
};  // namespace Fields

class HouseModeArmedMigration : public Migration {
public:
    explicit HouseModeArmedMigration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_45_house_mode_armed
}  // namespace MigrationTool
}  // namespace HubPlatform
