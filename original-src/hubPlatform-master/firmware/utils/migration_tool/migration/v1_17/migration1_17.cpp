#include "migration_tool/migration/v1_17/migration1_17.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_17 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto zigbeeDbName = "db.addons.zigbee.devices";

    auto allDevices = m_dbClient.findAll( zigbeeDbName, JSON_EMPTY_OBJ, nullptr );
    for( auto& device : allDevices ) {
        convertFiledsNames( device );

        for( auto& endpoint : device[ "endpoints" ] ) {
            convertFiledsNames( endpoint );

            for( auto& cluster : endpoint[ "clusters" ] ) {
                convertFiledsNames( cluster );

                for( auto& attr : cluster[ "attributes" ] ) {
                    convertFiledsNames( attr );
                }

                for( auto& command : cluster[ "commands" ] ) {
                    convertFiledsNames( command );
                }
            }
        }

        auto id = json_get_int( device, db::ID_FIELD_NAME );
        m_dbClient.remove( "db.addons.zigbee.devices", JSON( db::ID_FIELD_NAME << id ) );
        m_dbClient.insert( "db.addons.zigbee.devices", device );
    }

    return true;
}

void Migration::convertFiledsNames( JsonObject& object ) const {
    JsonObject result;
    for( auto it = object.begin(); it != object.end(); ++it ) {
        auto keyInSnakeCase = toSnakeCase( it.key() );
        result[ keyInSnakeCase ] = it.value();
    }
    object = std::move( result );
}

std::string Migration::toSnakeCase( const std::string& name ) const {
    std::string result;
    for( auto character : name ) {
        if( isupper( character ) ) {
            result += "_";
            result += tolower( character );
        }
        else {
            result += character;
        }
    }

    return result;
}

}  // namespace v1_17
}  // namespace MigrationTool
}  // namespace HubPlatform
