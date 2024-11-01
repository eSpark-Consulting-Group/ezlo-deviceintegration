#pragma once

#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_53_zigbee_gateway_settings {

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;

private:
    IDbClient& m_dbClient;
};

}  // namespace v1_53_zigbee_gateway_settings
}  // namespace MigrationTool
}  // namespace HubPlatform
