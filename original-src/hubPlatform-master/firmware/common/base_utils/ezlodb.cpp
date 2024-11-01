
#include "base_utils/ezlodb.h"

#include "database_client/dbid.h"
#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "json_utils/json_utils.h"
#include "base_utils/timeutils.h"

void EzloDb::insert( const std::string& dbName, const Json& obj ) {
    getClient().insert( dbName, obj );
}

void EzloDb::update( const std::string& dbName, const Json& query, const Json& obj, bool upsert, bool multi ) {
    getClient().update( dbName, query, obj, upsert, multi );
}

Json EzloDb::findOne( const std::string& dbName, const Query& query, const Json* fieldsToReturn ) {
    return getClient().findOne( dbName, query, fieldsToReturn );
}

std::vector< Json > EzloDb::findAll( const std::string& dbName,
                                     const Query& query,
                                     const Json* fieldsToReturn,
                                     int nToReturn,
                                     int nToSkip ) {
    return getClient().findAll( dbName, query, fieldsToReturn, nToReturn, nToSkip );
}

size_t EzloDb::count( const std::string& dbName, const Json query ) {
    return getClient().count( dbName, query );
}

bool EzloDb::has( const std::string& dbName, const Json query ) {
    return getClient().has( dbName, query );
}

void EzloDb::remove( const std::string& dbName, const Json& query, bool justOne ) {
    getClient().remove( dbName, query, justOne, false );
}

void EzloDb::capped( const std::string& dbName, int maxCollectionSize ) {
    getClient().capped( dbName, maxCollectionSize );
}

void EzloDb::clean() {
    getClient().clean();
}

void EzloDb::fsync() {
    getClient().fsync();
}

void EzloDb::lock() {
    getClient().lock();
}

void EzloDb::unlock() {
    getClient().unlock();
}

void EzloDb::connect( const int tries ) {
    return getClient().connect( tries );
}

bool EzloDb::connected() {
    return getClient().connected();
}

IDbClient& EzloDb::getClient() {
    static std::unique_ptr< IDbClient > client = createDbClient();
    return *client;
}

const std::string Db::genOid() {
    return db::DbId::genOID();
}

Json Db::getIdObj( const std::string& id ) {
    return JSON( "_id" << id );
}

bool Db::getOid( const std::string& id, std::string& oid ) {
    oid = id;
    return ( id.size() == idSize );
}

const std::string Db::getId( const Json& obj ) {
    try {
        return obj.at( "_id" ).get< std::string >();
    }
    catch( ... ) {
        return std::string();
    }
}

void Db::setField( const std::string& dbName, const std::string& id, const Json& queryAdd, const Json& value ) {
    update( dbName, JSON( queryAdd << "_id" << id ), JSON( "$set" << value ), true );
}

std::string Db::jsTime() {
    return TimeUtils::getCurrentIsoDate();
}
time_t Db::getTime() {
    return TimeUtils::getCurrentTime();
}

std::string Db::date( time_t time ) {
    return TimeUtils::timestampToIsoDate( time );
}

int Db::diffTime( const std::string& time ) {
    return TimeUtils::difftimeISODate( time );
}

void Db::insertWithAge( const std::string& dbName, const Json& json ) {
    insert( dbName, JSON( json << "age" << jsTime() ) );
}

Json Db::getList( const std::string& dbName,
                  std::function< bool( const std::string& ) > checkBack,
                  const EzloDb::Query query,
                  const Json* fieldsToReturn,
                  int nToReturn,
                  int nToSkip ) {
    Json result = JSON_EMPTY_ARR;
    for( const Json& element : findAll( dbName, query, fieldsToReturn, nToReturn, nToSkip ) )
        if( checkBack( getId( element ) ) )
            result.push_back( element );
    return result;
}

void Db::setFlag( const std::string& flag ) {
    update( SysFlagsDBName, JSON_EMPTY_OBJ, JSON( "_id" << flag << "value" << true ), true );
}

void Db::clrFlag( const std::string& flag ) {
    update( SysFlagsDBName, JSON_EMPTY_OBJ, JSON( "_id" << flag << "value" << false ), true );
}

bool Db::getFlag( const std::string& flag ) {
    return ( has( SysFlagsDBName, JSON( "_id" << flag << "value" << true ) ) );
}

Json Db::getFromArray( const Json& jsonObj,
                       const std::string& array,
                       const std::string& field,
                       const std::string& value ) {
    Json jsonArray = JSON_EMPTY_ARR;
    json_get_arr( jsonObj, array, jsonArray );
    for( const auto& element : jsonArray )
        if( element.is_object() && element.count( field ) && json_get_str( element, field ) == value )
            return element;
    return JSON_EMPTY_OBJ;
}

Json Db::getFieldsToReturn( const Json& fields ) {
    Json fieldsToReturn = JSON_EMPTY_OBJ;
    for( auto it = fields.cbegin(); it != fields.cend(); ++it )
        fieldsToReturn[ it.key() ] = true;
    return fieldsToReturn;
}

void Db::connect( const int tries ) {
    EzloDb::connect( tries );
}

bool Db::connected() {
    return EzloDb::connected();
}
