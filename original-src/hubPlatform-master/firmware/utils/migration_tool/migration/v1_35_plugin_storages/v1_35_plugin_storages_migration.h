#pragma once

#include "migration_tool/migration/migration.h"

/*

CHANGELOG:
    - find all cameras and populate the "cameras" storage/array with ids:
    - a camera is a device that has a "make_recording" item with setter functionality.

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_35_plugin_storages {

class PluginStoragesMigration : public HubPlatform::MigrationTool::Migration {
public:
    explicit PluginStoragesMigration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_35_plugin_storages
}  // namespace MigrationTool
}  // namespace HubPlatform
