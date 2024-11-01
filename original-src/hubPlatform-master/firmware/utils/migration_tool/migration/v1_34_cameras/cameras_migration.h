#pragma once

#include "migration_tool/migration/migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

/*

CHANGELOG:
    - find all cameras and populate the "cameras" storage/array with ids:
    - a camera is a device that has a "make_recording" item with setter functionality.

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_34_cameras {

class CamerasMigration : public HubPlatform::MigrationTool::Migration {
public:
    explicit CamerasMigration( IDbClient& dbClient );
    bool migrate() override;
    int recordsModified() const;

private:
    IDbClient& m_dbClient;
    int m_recordsModified{ 0 };
};

}  // namespace v1_34_cameras
}  // namespace MigrationTool
}  // namespace HubPlatform