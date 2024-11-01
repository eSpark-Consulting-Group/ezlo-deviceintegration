#include "acqured_plugins_fix_migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_42_acquired_plugins {

namespace DbNames {
const char AnonymousPlugins[] = "db.firmware.extensions_manager.anonymous_plugins";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char Acquired[] = "acquired";
const char Scenes[] = "scenes";
const char References[] = "references";
};  // namespace Fields

AcquiredPluginsMigration::AcquiredPluginsMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool AcquiredPluginsMigration::migrate() {
    LOGGER_DEBUG_RAW( "AcquiredPluginsMigration::migrate called" );

    auto plugins = m_dbClient.findAll( DbNames::AnonymousPlugins, JSON_EMPTY_OBJ, nullptr );
    for( auto& plugin : plugins ) {
        const std::string pluginId = json_get_str( plugin, Fields::id );
        JsonObject references;
        references[ Fields::References ][ Fields::Scenes ] = JSON_EMPTY_OBJ;
        m_dbClient.update( DbNames::AnonymousPlugins,
                           JSON( Fields::id << pluginId ),
                           JSON( "$set" << JSON( Fields::Acquired << 0 << references ) ) );
    }

    return true;
}

}  // namespace v1_42_acquired_plugins
}  // namespace MigrationTool
}  // namespace HubPlatform
