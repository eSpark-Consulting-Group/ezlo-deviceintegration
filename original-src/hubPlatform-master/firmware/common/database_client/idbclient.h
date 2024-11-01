#pragma once

#include "database_client/dbquery.h"
#include "database_client/dbtools.h"
#include "database_client/db_tables_names.h"

#include <string>
#include <vector>
#include <set>

class IDbClient {
public:
    virtual ~IDbClient() = default;

public:
    void insert( const std::string& dbName, const Json& obj );
    virtual void insert( const std::string& dbName, const Json& obj, const bool lockIgnore ) = 0;
    virtual void update( const std::string& dbName,
                         const Json& query,
                         const Json& obj,
                         bool upsert = false,
                         bool multi = false ) = 0;
    virtual void update( const std::string& dbName, const Json& query, std::function< void( Json& ) > updater ) = 0;
    Json findOne( const std::string& dbName, const db::Query& query, const Json* fieldsToReturn ) const;
    std::vector< Json > findAll( const std::string& dbName,
                                 const db::Query& query,
                                 const Json* fieldsToReturn,
                                 int nToReturn,
                                 int nToSkip ) const;
    std::vector< Json > findAll( const std::string& dbName, const db::Query& query, const Json* fieldsToReturn ) const;
    virtual std::vector< Json > findAll( const std::string& dbName,
                                         const db::Query& query,
                                         const Json* fieldsToReturn,
                                         int nToReturn,
                                         int nToSkip,
                                         const bool lockIgnore ) const;
    virtual size_t count( const std::string& dbName, const Json& query ) const;
    virtual bool has( const std::string& dbName, const Json& query ) const;
    void remove( const std::string& dbName, const Json& query );
    virtual void remove( const std::string& dbName, const Json& query, bool justOne, const bool lockIgnore ) = 0;
    void removeAll( const std::set< std::string >& excluded );
    virtual void capped( const std::string& dbName, int maxCollectionSize ) = 0;
    virtual bool exist( const std::string& dbName ) const = 0;
    virtual void create( const std::string& dbName ) = 0;
    virtual void clear( const std::string& dbName ) = 0;
    virtual void drop( const std::string& dbName ) = 0;
    virtual void clean() = 0;
    virtual void fsync() = 0;
    virtual void reset() = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual void resetChangesCounter() = 0;
    virtual void fixBrokenPrimaryKeys( const std::string& dbName ) = 0;
    virtual std::vector< std::string > getPrimaryKeys( const std::string& dbName ) const = 0;
    std::vector< std::string > getAllNames() const;
    std::vector< std::string > getAllNames( const std::set< std::string >& excludedDb,
                                            const bool onlyWithChanges ) const;
    std::vector< std::string > getDatabaseNames() const;
    std::vector< std::string > getCollectionNames( const std::string& dbName ) const;
    std::string dumpCollection( const std::string& dbName, int indentSpacesAmount = -1 ) const;
    bool restoreCollection( const std::string& dbName, const std::string& collection );

    /* @brief returns space (in bytes, multiple to a page size) currently allocated (mmaped) for a table with a @c
     * dbName name */
    virtual std::size_t size( const std::string& dbName ) const = 0;
    virtual void connect( const int tries ) = 0;
    virtual bool connected() const = 0;

    virtual std::vector< std::string > getAllCollectionsNames( const bool onlyWithChanges ) const = 0;
    virtual void forEachNth( const std::string& dbName,
                             const Json& query,
                             const std::function< void( Json, const size_t ) >& func,
                             size_t nToReturn,
                             size_t nToSkip ) const = 0;

private:
    template < typename Condition >
    std::vector< std::string > getCollectionsNamesByCondition( const bool onlyWithChanges,
                                                               Condition&& condition ) const;
};

std::unique_ptr< IDbClient > createDbClient( std::string&& pathToDb,
                                             std::string&& temporaryDbPath = getTemporaryStorageFilePath() );

std::unique_ptr< IDbClient > createDbClient();
