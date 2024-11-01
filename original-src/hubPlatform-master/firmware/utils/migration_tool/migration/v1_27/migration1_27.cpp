#include "migration_tool/migration/v1_27/migration1_27.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

#include <map>
#include <string>
#include <vector>

namespace HubPlatform {
namespace MigrationTool {
namespace v1_27 {

namespace {
void addProtectField( std::vector< Json >& houseModes ) {
    const std::map< std::string, bool > defaultHouseModeProtectState = {
        { "Home", false },
        { "Away", true },
        { "Night", true },
        { "Vacation", true },
    };

    for( auto& houseMode : houseModes ) {
        const auto houseModeName = json_get_str( houseMode, "name" );
        auto it = defaultHouseModeProtectState.find( houseModeName );

        houseMode[ "protect" ] = it == defaultHouseModeProtectState.end() ? true : it->second;
    }
}
}  // namespace

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto houseModes = m_dbClient.findAll( m_dataTableName, JSON_EMPTY_OBJ, nullptr );

    addProtectField( houseModes );

    for( const auto& houseMode : houseModes ) {
        m_dbClient.insert( m_dataTableName, houseMode );
    }

    return true;
}

}  // namespace v1_27
}  // namespace MigrationTool
}  // namespace HubPlatform
