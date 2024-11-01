#pragma once

#include "database_client/idbclient.h"
#include "database_client/lmdb++.h"

namespace UT {
class LMDBClientTestSuite;
};

class LMDBClient;

class LMDBClient final : public IDbClient {
public:
    friend class LMDBClientFixture;
    LMDBClient();
    LMDBClient( const std::string& pathToDB, const std::string& pathToTemporaryDB );
    ~LMDBClient() override = default;

    using IDbClient::insert;
    void insert( const std::string& dbName, const Json& obj, const bool lockIgnore ) override;
    void update( const std::string& dbName,
                 const Json& query,
                 const Json& obj,
                 bool upsert = false,
                 bool multi = false ) override;
    void update( const std::string& dbName, const Json& query, std::function< void( Json& ) > updater ) override;
    void remove( const std::string& dbName, const Json& query, bool justOne, const bool lockIgnore ) override;
    void capped( const std::string& dbName, int maxCollectionSize ) override;
    bool exist( const std::string& dbName ) const override;
    void create( const std::string& dbName ) override;
    void clear( const std::string& dbName ) override;
    void drop( const std::string& dbName ) override;
    void clean() override;
    void fsync() override;
    void reset() override;
    void lock() override;
    void unlock() override;
    void resetChangesCounter() override;
    std::size_t size( const std::string& dbName ) const override;
    void connect( const int tries ) override;
    bool connected() const override;

    void fixBrokenPrimaryKeys( const std::string& dbName ) override;

    std::vector< std::string > getPrimaryKeys( const std::string& dbName ) const override;

    static const std::string createTemporaryTableName( const std::string& tableName );
    std::vector< std::string > getPermanentCollectionsNames() const;
    std::vector< std::string > getTemporaryCollectionsNames() const;

    std::vector< std::string > getAllCollectionsNames( const bool onlyWithChanges ) const override;
    void forEachNth( const std::string& dbName,
                     const Json& query,
                     const std::function< void( Json, const size_t ) >& func,
                     size_t nToReturn,
                     size_t nToSkip ) const override;

private:
    lmdb::Env& getEnvInstanceForTableName( const std::string& tableName );
    const lmdb::Env& getEnvInstanceForTableName( const std::string& tableName ) const;
    void cleanInstance( lmdb::Env& env );

    void multiInsert( const std::string& dbName, const std::vector< Json >& obj );

    void getAllCollectionsNames( const lmdb::Env& env, std::vector< std::string >& names ) const;

private:
    lmdb::Env m_env;
    lmdb::Env m_tempEnv;
};
