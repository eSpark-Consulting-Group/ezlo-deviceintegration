
#include "base_utils/dblists.h"
#include "base_utils/acl.h"

JsonArray getLimitedResult( const JsonObject& params,
                            const Resource::Resources resource,
                            const std::string& dbName,
                            const Db::Query query,
                            const Json* fieldsToReturn,
                            int nToReturn,
                            int nToSkip ) {
    return Db::getList( dbName,
                        std::bind( &Acl::checkPermission, std::placeholders::_1, params, resource, Permission::VIEW ),
                        query,
                        fieldsToReturn,
                        nToReturn,
                        nToSkip );
}

void getList( const JsonObject& params,
              JsonObject& answer,
              const Resource::Resources resource,
              const std::string& dbName,
              const Db::Query query ) {
    answer[ "result" ] = getLimitedResult( params, resource, dbName, query );
}

bool getListById( const JsonObject& params,
                  JsonObject& answer,
                  const Resource::Resources resource,
                  const std::string& dbName ) {
    std::string id;
    if( !getField( params, "_id", answer, id ) )
        return false;
    getList( params, answer, resource, dbName, Db::getIdObj( id ) );
    return true;
}
