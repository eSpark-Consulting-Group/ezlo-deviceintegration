#pragma once

#include "database_client/idbclient.h"
#include "json_utils/json_fwd.h"

#include <map>
#include <mutex>

namespace HubPlatform {
namespace UT {

/* @brief fake IDbClient implementation for unit tests, uses an std::map(RAM) as a backend storage */
class FakeDbClient : public IDbClient {
public:
    ~FakeDbClient() override;

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

private:
    std::vector< std::string > getAllCollectionsNames( const bool onlyWithChanges ) const override;
    void forEachNth( const std::string& dbName,
                     const Json& query,
                     const std::function< void( Json, const size_t ) >& func,
                     size_t nToReturn,
                     size_t nToSkip ) const override;

private:
    /* real implementation of an IDbClient uses only strings as keys */
    using Key = std::string;
    using Value = Json;
    using NamedDb = std::map< Key, Value >;
    using NamedDbName = std::string;

    NamedDb& getNamedDb( const NamedDbName& dbName );
    const NamedDb* checkNamedDb( const NamedDbName& dbName ) const;
    void multiInsert( const std::string& dbName, const std::vector< Value >& jsonObjList );

private:
    mutable std::mutex m_dataBaseMtx; /* real implementation of an IDbClient is thread-safe */
    std::map< NamedDbName, NamedDb > m_dataBase;
};

}  // namespace UT
}  // namespace HubPlatform
