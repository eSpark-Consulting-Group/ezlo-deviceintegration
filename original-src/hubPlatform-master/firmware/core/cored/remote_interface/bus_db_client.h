#pragma once

#include "database_client/idbclient.h"
#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {

class BusDbClient : public IDbClient {
public:
    explicit BusDbClient( Services::Service& hostService )
    : m_hostService( hostService ) {}

    ~BusDbClient() override = default;

    using IDbClient::insert;
    void insert( const std::string& dbName, const Json& obj, const bool lockIgnore ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "insert"
                                            << "dbName" << dbName << "obj" << obj << "lockIgnore" << lockIgnore ) );
    }

    void update( const std::string& dbName,
                 const Json& query,
                 const Json& obj,
                 bool upsert = false,
                 bool multi = false ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "update"
                                            << "dbName" << dbName << "query" << query << "obj" << obj << "upsert"
                                            << upsert << "multi" << multi ) );
    }

    void update( const std::string& dbName, const Json& query, std::function< void( Json& ) > updater ) override {
        throw std::logic_error( "Method not implemented" );
    }

    void remove( const std::string& dbName, const Json& query, bool justOne, const bool lockIgnore ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "remove"
                                            << "dbName" << dbName << "query" << query << "justOne" << justOne
                                            << "lockIgnore" << lockIgnore ) );
    }

    void capped( const std::string& dbName, int maxCollectionSize ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "capped"
                                            << "dbName" << dbName << "maxCollectionSize" << maxCollectionSize ) );
    }

    bool exist( const std::string& dbName ) const override {
        auto result = m_hostService.sendRequest( GroupsNames::CORED,
                                                 MessageSubClasses::REQUEST_DB_DATA,
                                                 JSON( "operation"
                                                       << "exist"
                                                       << "dbName" << dbName ) );
        return json_get_bool( result, "exist" );
    }

    void create( const std::string& dbName ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "create"
                                            << "dbName" << dbName ) );
    }

    void clear( const std::string& dbName ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "clear"
                                            << "dbName" << dbName ) );
    }

    void drop( const std::string& dbName ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "drop"
                                            << "dbName" << dbName ) );
    }

    void clean() override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "clean" ) );
    }

    void fsync() override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "fsync" ) );
    }

    void reset() override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "reset" ) );
    }

    void lock() override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "lock" ) );
    }

    void unlock() override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "unlock" ) );
    }

    void resetChangesCounter() override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "resetChangesCounter" ) );
    }

    std::size_t size( const std::string& dbName ) const override {
        auto result = m_hostService.sendRequest( GroupsNames::CORED,
                                                 MessageSubClasses::REQUEST_DB_DATA,
                                                 JSON( "operation"
                                                       << "size"
                                                       << "dbName" << dbName ) );
        return json_get_int( result, "size" );
    }

    void connect( const int tries ) override {
        // do nothing.
    }

    bool connected() const override {
        return true;
    }

    void fixBrokenPrimaryKeys( const std::string& dbName ) override {
        m_hostService.sendMethodCall( GroupsNames::CORED,
                                      MessageSubClasses::PERFORM_DB_OPERATION,
                                      JSON( "operation"
                                            << "fixBrokenPrimaryKeys"
                                            << "dbName" << dbName ) );
    }

    std::vector< std::string > getPrimaryKeys( const std::string& dbName ) const override {
        auto result = m_hostService.sendRequest( GroupsNames::CORED,
                                                 MessageSubClasses::REQUEST_DB_DATA,
                                                 JSON( "operation"
                                                       << "getPrimaryKeys"
                                                       << "dbName" << dbName ) );
        return json_get_vec< std::string >( result, "primaryKeys" );
    }

private:
    std::vector< std::string > getAllCollectionsNames( const bool onlyWithChanges ) const override {
        auto result = m_hostService.sendRequest( GroupsNames::CORED,
                                                 MessageSubClasses::REQUEST_DB_DATA,
                                                 JSON( "operation"
                                                       << "getAllCollectionsNames"
                                                       << "onlyWithChanges" << onlyWithChanges ) );
        return json_get_vec< std::string >( result, "allCollectionsNames" );
    }

    void forEachNth( const std::string& dbName,
                     const Json& query,
                     const std::function< void( Json, const size_t ) >& func,
                     size_t n,
                     size_t nToSkip ) const override {
        auto result = m_hostService.sendRequest( GroupsNames::CORED,
                                                 MessageSubClasses::REQUEST_DB_DATA,
                                                 JSON( "operation"
                                                       << "getRecords"
                                                       << "dbName" << dbName << "query" << query << "n" << n
                                                       << "nToSkip" << nToSkip ) );
        Json records = JSON_EMPTY_ARR;
        json_get_arr( result, "records", records );
        for( const auto& record : records ) {
            func( record, std::numeric_limits< size_t >::max() );
        }
    }

private:
    Services::Service& m_hostService;
};

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
