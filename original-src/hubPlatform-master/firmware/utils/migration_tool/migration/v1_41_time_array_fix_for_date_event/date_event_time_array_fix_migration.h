#pragma once

#include "migration_tool/migration/migration.h"
#include "json_utils/json_utils.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_41_date_event_time_array_fix {

/*

CHANGELOG:
    - Fix value of "time" parameter for "isDate" event of "weekly" and "monthly" types to a correct array format.

*/

const char* const ScenesDb = "db.firmware.scenes_manager.rules";

class DateEventTimeFixMigration : public MigrationTool::Migration {
public:
    explicit DateEventTimeFixMigration( IDbClient& dbClient );
    bool migrate() override;

private:
    void processScene( Json& scene );
    void processBlock( Json& block, const std::string& keyPath );
    void iterateCompoundBlockFields( Json& fields, const std::string& keyPath );

    void processDateEventBlock( Json& block );
    static bool isTimeEntryValid( const Json& entry );

    void saveScene( const Json& updatedScene );

    static std::string fetchBlockMethodName( const Json& scene );
    static bool isOperatorBlockWithChildren( const std::string& methodName );

private:
    IDbClient& m_dbClient;
    int m_updatedBlocksCount;
};

}  // namespace v1_41_date_event_time_array_fix
}  // namespace MigrationTool
}  // namespace HubPlatform
