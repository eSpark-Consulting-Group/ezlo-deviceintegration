#pragma once

#include "migration_tool/system_repair/system_fix.h"
#include "database_client/idbclient.h"

#include <vector>
#include <memory>

namespace HubPlatform {
namespace MigrationTool {

class SystemRepairManager final {
public:
    SystemRepairManager( IDbClient& dbClient );

    bool repair();

private:
    IDbClient& m_dbClient;
    std::vector< std::unique_ptr< SystemFix > > m_systemFixes;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
