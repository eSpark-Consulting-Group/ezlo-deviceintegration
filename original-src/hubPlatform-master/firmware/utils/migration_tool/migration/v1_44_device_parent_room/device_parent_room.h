#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_44_device_parent_room {

namespace DbNames {
extern const char DevicesDBName[];
}  // namespace DbNames

namespace Fields {
extern const char id[];
};  // namespace Fields

class DeviceParentRoomMigration : public MigrationTool::Migration {
public:
    explicit DeviceParentRoomMigration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_44_device_parent_room
}  // namespace MigrationTool
}  // namespace HubPlatform
