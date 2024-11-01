#include "device_group_new_fields.h"

#include "migration_tool/utils.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_50_device_group_new_fields {

namespace DbNames {

const char HaDeviceGroupsDBName[] = "db.firmware.cored.device_groups";

}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char entryDelay[] = "entryDelay";
const char followEntry[] = "followEntry";
const char role[] = "role";
const char packageId[] = "packageId";
};  // namespace Fields

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    using namespace DbNames;
    LOGGER_DEBUG_RAW( "Device group new fields::migrate called" );

    auto deviceGroups = m_dbClient.findAll( HaDeviceGroupsDBName, JSON_EMPTY_OBJ, nullptr );

    for( auto& singleGroup : deviceGroups ) {
        singleGroup[ Fields::entryDelay ] = "none";
        singleGroup[ Fields::followEntry ] = false;
        singleGroup[ Fields::role ] = "user";
        singleGroup[ Fields::packageId ] = "";
        const std::string id = json_get_str( singleGroup, Fields::id );

        m_dbClient.update( HaDeviceGroupsDBName, JSON( Fields::id << id ), singleGroup );
    }
    return true;
}

}  // namespace v1_50_device_group_new_fields
}  // namespace MigrationTool
}  // namespace HubPlatform