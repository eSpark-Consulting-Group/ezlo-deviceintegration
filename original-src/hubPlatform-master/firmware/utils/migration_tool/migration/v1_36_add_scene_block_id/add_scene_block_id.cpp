#include "migration_tool/migration/v1_36_add_scene_block_id/add_scene_block_id.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "base_utils/ezlodb.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_36 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    const char ScenesDBName[] = "db.firmware.scenes_manager.rules";

    auto scenes = m_dbClient.findAll( ScenesDBName, JSON_EMPTY_OBJ, nullptr );

    for( auto& scene : scenes ) {
        try {
            auto& whenBlocks = scene[ "when" ];
            for( auto& block : whenBlocks ) {
                updateId( block );
            }
            auto& thenBlocks = scene[ "then" ];
            for( auto& block : thenBlocks ) {
                updateId( block );
            }
            m_dbClient.update( ScenesDBName, JSON( "_id" << json_get_str( scene, "_id" ) ), scene );
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "Migration error: " ) + cc::note( ex.what() ) );
        }
    }

    return true;
}

void Migration::updateId( Json& block ) {
    auto& id = block[ "_id" ];
    if( id.empty() ) {
        id = Db::genOid();
    }
}

}  // namespace v1_36
}  // namespace MigrationTool
}  // namespace HubPlatform
