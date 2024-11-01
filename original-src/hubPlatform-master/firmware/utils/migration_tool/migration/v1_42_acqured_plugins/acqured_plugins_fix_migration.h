#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_42_acquired_plugins {

namespace DbNames {
extern const char AnonymousPlugins[];
}  // namespace DbNames

namespace Fields {
extern const char id[];
};  // namespace Fields

class AcquiredPluginsMigration : public MigrationTool::Migration {
public:
    explicit AcquiredPluginsMigration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_42_acquired_plugins
}  // namespace MigrationTool
}  // namespace HubPlatform
