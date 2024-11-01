#include "vidoo_camera_devices_migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_51_vidoo_camera_devices {

namespace DbNames {
const char CoreDevicesDBName[] = "db.firmware.cored.devices";
}  // namespace DbNames

namespace Fields {
const char* const id = "_id";
const char* const type = "type";
const char* const parentDeviceId = "parentDeviceId";
};  // namespace Fields

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    LOGGER_DEBUG_RAW( "Vidoo camera devices migration::migrate called" );

    auto vidooCameraDevices =
        m_dbClient.findAll( DbNames::CoreDevicesDBName, JSON( Fields::type << "vidoo_generic_cam" ), nullptr );

    for( auto& device : vidooCameraDevices ) {
        device[ Fields::parentDeviceId ] = "";
        const std::string id = json_get_str( device, Fields::id );
        m_dbClient.update( DbNames::CoreDevicesDBName, JSON( Fields::id << id ), device );
    }

    return true;
}

}  // namespace v1_51_vidoo_camera_devices
}  // namespace MigrationTool
}  // namespace HubPlatform
