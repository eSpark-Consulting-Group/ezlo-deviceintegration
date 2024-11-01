#include "migration_tool/migration/v1_35_plugin_storages/v1_35_plugin_storages_migration.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"

#include <numeric>

namespace HubPlatform {
namespace MigrationTool {
namespace v1_35_plugin_storages {

namespace {
size_t fixJsonKeys( Json& json ) {
    if( json.is_array() ) {
        return std::accumulate( json.begin(), json.end(), size_t( 0 ), []( size_t result, Json& element ) {
            return result + fixJsonKeys( element );
        } );
    }
    else if( json.is_object() ) {
        size_t result = 0;

        std::vector< std::string > keysToFix;
        for( auto it = json.begin(); it != json.end(); ++it ) {
            auto key = it.key();
            if( !key.empty() && key.front() == '\\' ) {
                keysToFix.push_back( std::move( key ) );
            }

            result += fixJsonKeys( it.value() );
        }

        for( auto& key : keysToFix ) {
            auto element = std::move( json[ key ] );

            json.erase( key );
            key.insert( key.begin(), '\\' );

            json[ std::move( key ) ] = std::move( element );
        }

        return result + keysToFix.size();
    }

    return 0;
}

bool isPluginStorageDb( const std::string& dbName ) {
    static const std::string prefix = "db.plugins.";
    static const std::string suffix = ".storage";

    return dbName.size() > prefix.size() + suffix.size() && dbName.compare( 0, prefix.size(), prefix ) == 0 &&
           dbName.compare( dbName.size() - suffix.size(), suffix.size(), suffix ) == 0;
};
}  // namespace

PluginStoragesMigration::PluginStoragesMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool PluginStoragesMigration::migrate() {
    LOGGER_DEBUG_RAW( "PluginStoragesMigration::migrate called" );

    for( const auto& dbName : m_dbClient.getAllNames() ) {
        if( isPluginStorageDb( dbName ) ) {
            for( const auto& key : m_dbClient.getPrimaryKeys( dbName ) ) {
                const auto query = JSON( db::ID_FIELD_NAME << key );
                auto result = m_dbClient.findOne( dbName, query, nullptr );

                if( fixJsonKeys( result[ "value" ] ) ) {
                    LOGGER_DEBUG( "Fix table key " + key + " in table " + dbName );
                    m_dbClient.update( dbName, query, result );
                }
            }
        }
    }

    return true;
}

}  // namespace v1_35_plugin_storages
}  // namespace MigrationTool
}  // namespace HubPlatform
