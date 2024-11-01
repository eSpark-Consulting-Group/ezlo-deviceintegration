
#include "migration_tool/utils.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace MigrationTool {
namespace Utils {

bool renameCollections( IDbClient& dbClient, const Renames& renames ) {
    for( auto& rename : renames ) {
        if( !renameCollection( dbClient, rename.first, rename.second ) )
            return false;
    }

    return true;
}

bool renameCollection( IDbClient& dbClient, const std::string& oldName, const std::string& newName ) {
    LOGGER_TRACE( "Trying to rename " + oldName + " to " + newName );
    if( !dbClient.exist( oldName ) ) {
        LOGGER_TRACE( "There is no collection " + oldName + ". Nothing to do" );
        return true;
    }

    auto content = dbClient.findAll( oldName, JSON_EMPTY_OBJ, nullptr, 0, 0, true );

    if( dbClient.exist( newName ) ) {
        LOGGER_DEBUG( "Collection " + newName + " should not exist at the moment, clearing its content..." );
        dbClient.clear( newName );
    }
    else {
        if( content.empty() ) {
            dbClient.create( newName );
        }
    }

    for( const auto& element : content )
        dbClient.insert( newName, element, true );

    dbClient.drop( oldName );

    LOGGER_TRACE( "Collection renamed" );
    return true;
}

}  // namespace Utils
}  // namespace MigrationTool
}  // namespace HubPlatform
