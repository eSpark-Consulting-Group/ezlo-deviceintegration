#include "cameras_migration.h"
#include "resources.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_34_cameras {

CamerasMigration::CamerasMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool CamerasMigration::migrate() {
    LOGGER_DEBUG_RAW( "CamerasMigration::migrate called" );
    auto items = m_dbClient.findAll(
        Tables::ItemsDb, JSON( Fields::name << Values::makeRecording << Fields::hasSetter << true ), nullptr );
    if( items.empty() ) {
        LOGGER_TRACE( "Nothing to migrate" );
        return true;
    }

    for( const auto& item : items ) {
        auto deviceId = json_get_str( item, Fields::deviceId );
        auto itemId = json_get_str( item, Fields::id );

        m_dbClient.update( Tables::HouseModesCamerasDb,
                           JSON_EMPTY_OBJ,
                           JSON( Fields::id << deviceId << Values::makeRecording << itemId ),
                           true );
        m_recordsModified++;
    }

    return true;
}

int CamerasMigration::recordsModified() const {
    return m_recordsModified;
}

}  // namespace v1_34_cameras
}  // namespace MigrationTool
}  // namespace HubPlatform