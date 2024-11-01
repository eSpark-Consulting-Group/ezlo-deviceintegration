#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_50_device_group_new_fields {

namespace DbNames {
extern const char HaDeviceGroupsDBName[];
}  // namespace DbNames

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_50_device_group_new_fields
}  // namespace MigrationTool
}  // namespace HubPlatform
