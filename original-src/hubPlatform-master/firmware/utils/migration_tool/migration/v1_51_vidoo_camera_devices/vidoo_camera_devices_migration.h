#pragma once
#pragma once

#include "migration_tool/migration/migration.h"

/*

CHANGELOG:
    - Remove parent device from vidoo camera devices
*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_51_vidoo_camera_devices {

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_51_vidoo_camera_devices
}  // namespace MigrationTool
}  // namespace HubPlatform
