#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_43_script_timeout {

namespace DbNames {
extern const char CoreSettingsDBName[];
}  // namespace DbNames

namespace Fields {
extern const char id[];
};  // namespace Fields

class ScriptTimeoutMigration : public MigrationTool::Migration {
public:
    explicit ScriptTimeoutMigration( IDbClient& dbClient );
    bool migrate() override;
    int getConfigTimeoutValue( const std::string& key );

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_43_script_timeout
}  // namespace MigrationTool
}  // namespace HubPlatform
