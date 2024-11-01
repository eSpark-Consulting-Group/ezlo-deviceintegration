#include "device_parent_room.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_44_device_parent_room {

namespace DbNames {
const char DevicesDBName[] = "db.firmware.cored.devices";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
};  // namespace Fields

DeviceParentRoomMigration::DeviceParentRoomMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool DeviceParentRoomMigration::migrate() {
    LOGGER_DEBUG_RAW( "DeviceParentRoomMigration::migrate called" );

    const std::string ParentRoom = "parentRoom";

    auto devices = m_dbClient.findAll( DbNames::DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto device : devices ) {
        std::string parentId;
        if( json_get( device, "parentDeviceId", parentId ) ) {
            if( !parentId.empty() ) {
                std::string id;
                if( json_get( device, "_id", id ) ) {
                    m_dbClient.update(
                        DbNames::DevicesDBName, JSON( "_id" << id ), JSON( "$set" << JSON( ParentRoom << true ) ) );
                }
            }
        }
    }
    return true;
}

}  // namespace v1_44_device_parent_room
}  // namespace MigrationTool
}  // namespace HubPlatform
