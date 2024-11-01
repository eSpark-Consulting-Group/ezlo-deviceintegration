#include "migration_tool/migration/v1_48_rename_grop_db_tables/group_db_rename_migration.h"

#include "migration_tool/utils.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_48 {

namespace DbNames {
const char HaScenesDeviceGroupsDBName[] = "db.firmware.scenes_manager.device_groups";
const char HaScenesItemGroupsDBName[] = "db.firmware.scenes_manager.item_groups";
const char HaDeviceGroupsDBName[] = "db.firmware.cored.device_groups";
const char HaItemGroupsDBName[] = "db.firmware.cored.item_groups";
const char HaWrongTableNameforItemGroupsDBName[] = "db.firmware.cored`.item_groups";
}  // namespace DbNames

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    using namespace DbNames;
    renameTable( HaScenesDeviceGroupsDBName, HaDeviceGroupsDBName );
    // rename scenes table to cored table
    renameTable( HaScenesItemGroupsDBName, HaItemGroupsDBName );

    // if already have wrong cored table, copy records from it to correct cored table
    if( copyTable( HaWrongTableNameforItemGroupsDBName, HaItemGroupsDBName ) ) {
        m_dbClient.drop( HaWrongTableNameforItemGroupsDBName );
    }

    return true;
}

void Migration::renameTable( const std::string& oldTable, const std::string& newTable ) {
    if( !m_dbClient.exist( oldTable ) )
        return;

    m_dbClient.create( newTable );
    copyTable( oldTable, newTable );
    m_dbClient.drop( oldTable );
}

bool Migration::copyTable( const std::string& oldTable, const std::string& newTable ) {
    if( !m_dbClient.exist( oldTable ) )
        return false;

    auto records = m_dbClient.findAll( oldTable, JSON_EMPTY_OBJ, nullptr );
    for( const auto& record : records ) {
        m_dbClient.insert( newTable, record );
    }
    return true;
}

}  // namespace v1_48
}  // namespace MigrationTool
}  // namespace HubPlatform
