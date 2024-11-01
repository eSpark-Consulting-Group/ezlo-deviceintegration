#pragma once
#pragma once

#include "migration_tool/migration/migration.h"
#include "config_reader/config_reader.h"

class IDbClient;

namespace HubPlatform {
class ConfigReader;
}

namespace HubPlatform {
namespace MigrationTool {
namespace v1_49_house_mode_entry_delay {

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient, ConfigReader* configReader = nullptr );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
    ConfigReader* m_configReaderPtr{ nullptr };
};

}  // namespace v1_49_house_mode_entry_delay
}  // namespace MigrationTool
}  // namespace HubPlatform
