#include "migration_tool/migration/v1_23/migration.h"

#include "migration_tool/utils.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_23 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto oldCameraStreamDb = "Cameras.cameras";
    auto newCameraStreamDb = "db.addons.cameras.streams";

    auto streams = m_dbClient.findAll( oldCameraStreamDb, JSON_EMPTY_OBJ, nullptr );
    m_dbClient.drop( oldCameraStreamDb );
    m_dbClient.create( newCameraStreamDb );
    for( const auto& stream : streams ) {
        m_dbClient.insert( newCameraStreamDb, stream );
    }

    auto dbTunnels = "db.firmware.networkd.cloud.ssh.tunnels";
    auto dbItems = "db.firmware.uid.items";

    Json fieldsToReturn = JSON( "_id" << 1 << "value" << 1 );
    auto items = m_dbClient.findAll( dbItems,
                                     JSON( "name"
                                           << "camera_stream" ),
                                     &fieldsToReturn );
    for( const auto& item : items ) {
        std::string localRtspUrl;
        json_get( item, std::vector< Json >{ "value", "localRtspUrl" }, localRtspUrl );
        auto streamIdPos = localRtspUrl.find_last_of( "/" );
        auto portPos = localRtspUrl.find_last_of( ":" );
        if( streamIdPos == std::string::npos || portPos == std::string::npos ) {
            LOGGER_ERROR( "incorrect localRtspUrl" );
            continue;
        }
        auto localPort = std::stoi( localRtspUrl.substr( portPos + 1, streamIdPos - 1 ) );
        auto id = json_get_str( item, "_id" );
        m_dbClient.insert( dbTunnels,
                           JSON( "identity" << id << "method"
                                            << "cloud.tunnel_camera_coordinator"
                                            << "token"
                                            << ""
                                            << "tokenExpirationDate" << 0 << "tunnelLink"
                                            << ""
                                            << "tunnelPort" << 0 << "localPort" << localPort ) );
    }

    return true;
}

}  // namespace v1_23
}  // namespace MigrationTool
}  // namespace HubPlatform
