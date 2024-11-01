#pragma once

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class FakeIDbClient : public IDbClient {
public:
    void insert( const std::string& dbName, const Json& obj, const bool lockIgnore ) override {}
    void update( const std::string& dbName, const Json& query, const Json& obj, bool upsert, bool multi ) override {}
    void update( const std::string& dbName, const Json& query, std::function< void( Json& ) > updater ) override {}
    std::vector< Json > findAll( const std::string& dbName,
                                 const db::Query& query,
                                 const Json* fieldsToReturn,
                                 int nToReturn,
                                 int nToSkip,
                                 const bool lockIgnore ) const override {
        return {};
    }
    size_t count( const std::string& dbName, const Json& query ) const override {
        return 0;
    }
    bool has( const std::string& dbName, const Json& query ) const override {
        return false;
    }
    void remove( const std::string& dbName, const Json& query, bool justOne, const bool lockIgnore ) override {}
    void capped( const std::string& dbName, int maxCollectionSize ) override {}
    bool exist( const std::string& dbName ) const override {
        return false;
    }
    void create( const std::string& dbName ) override {}
    void clear( const std::string& dbName ) override {}
    void drop( const std::string& dbName ) override {}
    void clean() override {}
    void fsync() override {}
    void reset() override {}
    void lock() override {}
    void unlock() override {}
    void resetChangesCounter() override {}
    std::size_t size( const std::string& dbName ) const override {
        return 0;
    }
    void connect( const int tries ) override {}
    bool connected() const override {
        return true;
    }

    void fixBrokenPrimaryKeys( const std::string& dbName ) override {}

    std::vector< std::string > getPrimaryKeys( const std::string& dbName ) const override {
        return {};
    }

private:
    std::vector< std::string > getAllCollectionsNames( const bool onlyWithChanges ) const override {
        return {};
    }
    void forEachNth( const std::string& dbName,
                     const Json& query,
                     const std::function< void( Json, const size_t ) >& func,
                     size_t n,
                     size_t nToSkip ) const {}
};

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
