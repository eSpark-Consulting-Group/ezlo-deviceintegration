
#pragma once

#include "json_utils/json_wrapper.h"
#include <string>
#include <vector>
#include <functional>
#include "database_client/dbquery.h"

class IDbClient;

class EzloDb {
public:
    using Query = db::Query;
    static void insert( const std::string& dbName, const Json& obj );
    static void update( const std::string& dbName,
                        const Json& query,
                        const Json& obj,
                        bool upsert = false,
                        bool multi = false );
    static Json findOne( const std::string& dbName, const Query& query, const Json* fieldsToReturn = nullptr );
    static std::vector< Json > findAll( const std::string& dbName,
                                        const Query& query,
                                        const Json* fieldsToReturn = nullptr,
                                        int nToReturn = 0,
                                        int nToSkip = 0 );
    static size_t count( const std::string& dbName, const Json query );
    static bool has( const std::string& dbName, const Json query );
    static void remove( const std::string& dbName, const Json& query, bool justOne = false );
    static void capped( const std::string& dbName, int maxCollectionSize = 0 );
    static void clean();
    static void fsync();
    static void lock();
    static void unlock();
    static void connect( const int tries = 1 );
    static bool connected();

private:
    EzloDb() {}

protected:
    static IDbClient& getClient();
};

class Db : public EzloDb {
public:
    static const std::string genOid();
    static Json getIdObj( const std::string& id );
    static bool getOid( const std::string& id, std::string& oid );
    static const std::string getId( const Json& obj );
    static void setField( const std::string& dbName, const std::string& id, const Json& queryAdd, const Json& value );
    static std::string jsTime();
    static time_t getTime();
    static std::string date( time_t time );
    static int diffTime( const std::string& time );
    static void insertWithAge( const std::string& dbName, const Json& json );
    static Json getList( const std::string& dbName,
                         std::function< bool( const std::string& id ) > checkBack,
                         const Query query,
                         const Json* fieldsToReturn = nullptr,
                         int nToReturn = 0,
                         int nToSkip = 0 );
    static void setFlag( const std::string& flag );
    static void clrFlag( const std::string& flag );
    static bool getFlag( const std::string& flag );
    static Json getFromArray( const Json& jsonObj,
                              const std::string& array,
                              const std::string& field,
                              const std::string& value );
    static Json getFieldsToReturn( const Json& fields );
    static void connect( const int tries = 1 );
    static bool connected();

    static const size_t idSize = 24;
};
