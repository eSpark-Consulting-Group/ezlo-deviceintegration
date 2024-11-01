#include "database_client/idbclient.h"
#include "database_client/lmdb_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/fake_db/fake_db_client.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"
#include "test_utils/temporary_folder_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

/*

TESTPLAN:

DONE    1. Naming
DONE        1.1 Get all names
DONE        1.2 Get database names
DONE        1.3 Get collection names
DONE    2. API
DONE        2.1 Insert
DONE        2.2 Insert without id
DONE        2.3 Get all names
DONE        2.4 Create
DONE        2.5 Exist
DONE        2.6 Clear
DONE        2.7 Drop
DONE        2.8 Clean
DONE        2.9 Has and count
DONE        2.10 Find any
DONE        2.11 Find one
DONE        2.12 Find all
DONE            2.12.1 Find all in range
DONE        2.13 Update
DONE        2.14 Fields to return
DONE        2.15 Find one and update
DONE        2.16 Append to array
DONE        2.17 Find one with array field
DONE        2.18 Complex update
DONE        2.19 forEachNth: should limit the result set size properly

*/

namespace HubPlatform {
namespace UT {
namespace {

using namespace ::testing;

/* TODO: move these tests to IDbClientTestSuite */
class StubIDbClient final : public IDbClient {
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

    std::vector< std::string > getAllCollectionsNames( const bool onlyWithChanges ) const override {
        std::vector< std::string > result = { "abc.def", "ab.x", "abcd.ab", "abc.xyz", "q" };
        if( !onlyWithChanges ) {
            result.push_back( "xy.abc" );
        }
        return result;
    }
    void forEachNth( const std::string& dbName,
                     const Json& query,
                     const std::function< void( Json, const size_t ) >& func,
                     size_t nToReturn,
                     size_t nToSkip ) const override {}

    void fixBrokenPrimaryKeys( const std::string& dbName ) override {}

    std::vector< std::string > getPrimaryKeys( const std::string& dbName ) const override {
        return {};
    }
};

class IDBClientTestSuite : public testing::Test {};

TEST_F( IDBClientTestSuite, getAllNames /*1.1*/ ) {
    StubIDbClient client;

    EXPECT_EQ( client.getAllNames(),
               std::vector< std::string >( { "abc.def", "ab.x", "abcd.ab", "abc.xyz", "q", "xy.abc" } ) );
    EXPECT_EQ( client.getAllNames( { "a", "b", "c" }, false ),
               std::vector< std::string >( { "abc.def", "ab.x", "abcd.ab", "abc.xyz", "q", "xy.abc" } ) );
    EXPECT_EQ( client.getAllNames( { "abc", "q" }, false ),
               std::vector< std::string >( { "abc.def", "ab.x", "abcd.ab", "abc.xyz", "xy.abc" } ) );
    EXPECT_EQ( client.getAllNames( { "abc.def", "q" }, true ),
               std::vector< std::string >( { "ab.x", "abcd.ab", "abc.xyz" } ) );
    EXPECT_EQ( client.getAllNames( { "ab.x", "xy.abc" }, false ),
               std::vector< std::string >( { "abc.def", "abcd.ab", "abc.xyz", "q" } ) );
    EXPECT_EQ( client.getAllNames( { "abc.def", "ab.x", "abcd.ab", "abc.xyz", "q", "xy.abc" }, false ),
               std::vector< std::string >() );
}

TEST_F( IDBClientTestSuite, getDatabaseNames /*1.2*/ ) {
    StubIDbClient client;

    EXPECT_EQ( client.getDatabaseNames(), std::vector< std::string >( { "ab", "abc", "abcd", "q", "xy" } ) );
}

TEST_F( IDBClientTestSuite, getCollectionNames /*1.3*/ ) {
    StubIDbClient client;

    EXPECT_EQ( client.getCollectionNames( "a" ), std::vector< std::string >() );
    EXPECT_EQ( client.getCollectionNames( "xy" ), std::vector< std::string >( { "xy.abc" } ) );
    EXPECT_EQ( client.getCollectionNames( "abc" ), std::vector< std::string >( { "abc.def", "abc.xyz" } ) );
}

template < typename T >
class IDbClientTestSuite : public Test {
protected:
    IDbClientTestSuite();

    ~IDbClientTestSuite() {
        m_dbclient.reset();
        m_storageFolderWrapper.reset();
    }

    void SetUp() override {
        getDbClient().clean();
    }

    void TearDown() override {
        getDbClient().clean();
    }

    IDbClient& getDbClient() {
        return *m_dbclient;
    }

private:
    std::unique_ptr< Tests::TemporaryFolderWrapper > m_storageFolderWrapper;
    std::unique_ptr< IDbClient > m_dbclient;
};

template <>
IDbClientTestSuite< LMDBClient >::IDbClientTestSuite() {
    m_storageFolderWrapper = std::make_unique< Tests::TemporaryFolderWrapper >();
    m_dbclient = createDbClient( m_storageFolderWrapper->getPath() + "/" + StorageFileName,
                                 m_storageFolderWrapper->getPath() + "/" + TemporaryStorageFileName );
}

template <>
IDbClientTestSuite< FakeDbClient >::IDbClientTestSuite() {
    m_dbclient = std::make_unique< FakeDbClient >();
}

using DbClientImplementationTypes = Types< LMDBClient, FakeDbClient >;
TYPED_TEST_CASE( IDbClientTestSuite, DbClientImplementationTypes );

TYPED_TEST( IDbClientTestSuite, testInsert /*2.1*/ ) {
    auto& dbClient = this->getDbClient();

    EXPECT_EQ( 0U, dbClient.count( "test", Json::object() ) );

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id" ) );
    EXPECT_EQ( 1U, dbClient.count( "test", Json::object() ) );
}

TYPED_TEST( IDbClientTestSuite, testInsertWithoutID /*2.2*/ ) {
    auto& dbClient = this->getDbClient();

    EXPECT_EQ( 0U, dbClient.count( "test", Json::object() ) );

    dbClient.insert( "test", JSON( "value" << 10 ) );
    auto result = dbClient.findOne( "test", Json::object(), nullptr );
    EXPECT_EQ( 1U, result.count( db::ID_FIELD_NAME ) );
}

TYPED_TEST( IDbClientTestSuite, testGetAllNames /*2.3*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test.1", JSON( db::ID_FIELD_NAME << "id" ) );
    dbClient.insert( "test.2", JSON( db::ID_FIELD_NAME << "id" ) );

    auto result = dbClient.getAllNames();
    EXPECT_THAT( result, ElementsAre( std::string( "test.1" ), std::string( "test.2" ) ) );
}

TYPED_TEST( IDbClientTestSuite, testCreate /*2.4*/ ) {
    auto& dbClient = this->getDbClient();

    EXPECT_TRUE( dbClient.getAllNames().empty() );

    dbClient.create( "test" );

    EXPECT_THAT( dbClient.getAllNames(), ElementsAre( std::string( "test" ) ) );
    EXPECT_EQ( 0U, dbClient.count( "test", Json::object() ) );
}

TYPED_TEST( IDbClientTestSuite, testExist /*2.5*/ ) {
    auto& dbClient = this->getDbClient();

    EXPECT_FALSE( dbClient.exist( "test.1" ) );
    EXPECT_FALSE( dbClient.exist( "test.2" ) );

    dbClient.create( "test.1" );

    EXPECT_TRUE( dbClient.exist( "test.1" ) );
    EXPECT_FALSE( dbClient.exist( "test.2" ) );

    dbClient.create( "test.2" );

    EXPECT_TRUE( dbClient.exist( "test.1" ) );
    EXPECT_TRUE( dbClient.exist( "test.2" ) );
}

TYPED_TEST( IDbClientTestSuite, testClear /*2.6*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id" ) );
    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id1" ) );

    EXPECT_THAT( dbClient.getAllNames(), ElementsAre( std::string( "test" ) ) );
    EXPECT_EQ( 2U, dbClient.count( "test", Json::object() ) );

    dbClient.clear( "test" );

    EXPECT_THAT( dbClient.getAllNames(), ElementsAre( std::string( "test" ) ) );
    EXPECT_EQ( 0U, dbClient.count( "test", Json::object() ) );
}

TYPED_TEST( IDbClientTestSuite, testDrop /*2.7*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test.1", JSON( db::ID_FIELD_NAME << "id" ) );
    dbClient.insert( "test.2", JSON( db::ID_FIELD_NAME << "id" ) );

    auto before = dbClient.getAllNames();
    EXPECT_THAT( before, ElementsAre( std::string( "test.1" ), std::string( "test.2" ) ) );

    dbClient.drop( "test.1" );

    auto after = dbClient.getAllNames();
    EXPECT_THAT( after, ElementsAre( std::string( "test.2" ) ) );
}

TYPED_TEST( IDbClientTestSuite, testClean /*2.8*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id" ) );
    dbClient.insert( "test_2", JSON( db::ID_FIELD_NAME << "id" ) );

    dbClient.clean();

    auto result = dbClient.getAllNames();
    EXPECT_TRUE( result.empty() );
}

TYPED_TEST( IDbClientTestSuite, testHasAndCount /*2.9*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id" ) );
    EXPECT_EQ( 1U, dbClient.count( "test", Json::object() ) );
    EXPECT_EQ( 1U, dbClient.count( "test", JSON( db::ID_FIELD_NAME << "id" ) ) );
    EXPECT_EQ( 0U, dbClient.count( "test", JSON( db::ID_FIELD_NAME << "id1" ) ) );

    EXPECT_TRUE( dbClient.has( "test", Json::object() ) );
    EXPECT_TRUE( dbClient.has( "test", JSON( db::ID_FIELD_NAME << "id" ) ) );
    EXPECT_FALSE( dbClient.has( "test", JSON( db::ID_FIELD_NAME << "id1" ) ) );

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id1" ) );
    EXPECT_EQ( 2U, dbClient.count( "test", Json::object() ) );
    EXPECT_EQ( 1U, dbClient.count( "test", JSON( db::ID_FIELD_NAME << "id" ) ) );
    EXPECT_EQ( 1U, dbClient.count( "test", JSON( db::ID_FIELD_NAME << "id1" ) ) );
    EXPECT_TRUE( dbClient.has( "test", JSON( db::ID_FIELD_NAME << "id1" ) ) );
}

TYPED_TEST( IDbClientTestSuite, testFindOne /*2.10*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id" ) );

    auto result = dbClient.findOne( "test", Json::object(), nullptr );
    EXPECT_EQ( result, JSON( db::ID_FIELD_NAME << "id" ) );
}

TYPED_TEST( IDbClientTestSuite, testFindOne2 /*2.11*/ ) {
    auto& dbClient = this->getDbClient();

    const JsonObject entry1 = { { "_id", "uid1" },
                                { "int_value", 10 },
                                { "literal_value", "string1" },
                                { "float_value", 3.14 },
                                { "object_value", { { "name", "First" } } } };
    dbClient.insert( "test", entry1 );

    const JsonObject entry2 = { { "_id", "uid2" },
                                { "int_value", 12 },
                                { "literal_value", "string2" },
                                { "float_value", 2.71 },
                                { "object_value", { { "name", "Second" } } } };
    dbClient.insert( "test", entry2 );

    const JsonObject entry3 = { { "_id", "uid2" },
                                { "int_value", 14 },
                                { "literal_value", "string3" },
                                { "float_value", 1.618 },
                                { "object_value", { { "name", "Third" } } } };
    dbClient.insert( "test", entry3 );

    auto result = dbClient.findOne( "test",
                                    JSON( "literal_value"
                                          << "string3" ),
                                    nullptr );
    const auto uid2Str = json_get_str( result, "_id" );
    EXPECT_STREQ( "uid2", uid2Str.c_str() );
}

TYPED_TEST( IDbClientTestSuite, testFindAll /*2.12*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id1" ) );
    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id2" ) );
    dbClient.insert( "test", JSON( db::ID_FIELD_NAME << "id3" ) );

    auto result = dbClient.findAll( "test", Json::object(), nullptr, 0, 0, false );
    EXPECT_EQ( 3U, result.size() );
}

TYPED_TEST( IDbClientTestSuite, testFindAllRange /*2.12.1*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test",
                     JSON( db::ID_FIELD_NAME << "id0"
                                             << "created" << 100 << "closed" << 0 ) );
    dbClient.insert( "test",
                     JSON( db::ID_FIELD_NAME << "id1"
                                             << "created" << 100 << "closed" << 105 ) );
    dbClient.insert( "test",
                     JSON( db::ID_FIELD_NAME << "id2"
                                             << "created" << 100 << "closed" << 110 ) );
    dbClient.insert( "test",
                     JSON( db::ID_FIELD_NAME << "id3"
                                             << "created" << 100 << "closed" << 120 ) );

    {
        auto result = dbClient.findAll( "test", Json::object(), nullptr, 0, 0, false );
        EXPECT_EQ( 4U, result.size() );
    }
    {
        auto result = dbClient.findAll( "test", JSON( "created" << JSON( "$gt" << 0 ) ), nullptr, 0, 0, false );
        EXPECT_EQ( 4U, result.size() ) << "created > 0";
    }
    {
        auto query = JSON( "created" << JSON( "$gt" << 0 ) << "closed" << JSON( "$gt" << 0 ) );
        auto result = dbClient.findAll( "test", query, nullptr, 0, 0, false );
        EXPECT_EQ( 3U, result.size() ) << "created > 0 && closed > 0";
    }
    {
        auto query = JSON( "created" << JSON( "$gt" << 0 ) << "closed" << JSON( "$gt" << 0 << "$lt" << 120 ) );
        auto result = dbClient.findAll( "test", query, nullptr, 0, 0, false );
        EXPECT_EQ( 2U, result.size() ) << "created > 0 && closed > 0 && closed < 120";
    }
    {
        auto query = JSON( "created" << JSON( "$gt" << 0 ) << "closed" << JSON( "$gt" << 0 << "$lt" << 120 ) );
        dbClient.remove( "test", query );
        EXPECT_EQ( dbClient.count( "test", Json::object() ), 2 );
    }
}

TYPED_TEST( IDbClientTestSuite, testUpdate /*2.13*/ ) {
    auto& dbClient = this->getDbClient();

    dbClient.insert( "test",
                     JSON( db::ID_FIELD_NAME << "id"
                                             << "value" << 1 ) );
    EXPECT_EQ( 1U, dbClient.count( "test", Json::object() ) );

    dbClient.update( "test", JSON( db::ID_FIELD_NAME << "id" ), JSON( "value" << 10 ), false, true );
    EXPECT_EQ( 1U, dbClient.count( "test", Json::object() ) );

    auto result = dbClient.findOne( "test", Json::object(), nullptr );
    EXPECT_EQ( result,
               JSON( db::ID_FIELD_NAME << "id"
                                       << "value" << 10 ) );

    dbClient.update( "test_3",
                     JSON_EMPTY_OBJ,
                     JSON( "id"
                           << "value" ),
                     true,
                     false );
    const JsonObject fields = JSON( "id" << 1 );
    auto upsert_result = dbClient.findOne( "test_3", Json::object(), &fields );
    EXPECT_EQ( upsert_result,
               JSON( "id"
                     << "value" ) );
}

TYPED_TEST( IDbClientTestSuite, testFieldsToReturn /*2.14*/ ) {
    auto& dbClient = this->getDbClient();

    JsonObject insertObject = R"({
                "value": "12345",
                "array": [],
                "value2": null
             })"_json;
    dbClient.insert( "test", insertObject );

    const JsonObject fieldsToReturn = JSON( "array" << true );
    JsonObject result = dbClient.findOne( "test",
                                          JSON( "value"
                                                << "12345" ),
                                          &fieldsToReturn );

    EXPECT_EQ( 1U, result.size() );
    EXPECT_TRUE( result[ "array" ].is_array() );
}

TYPED_TEST( IDbClientTestSuite, testFindOneAndUpdate /*2.15*/ ) {
    auto& dbClient = this->getDbClient();

    JsonObject insertObject = R"({
                "_id": "1",
                "value": "12345",
                "array": [],
                "value2": null
             })"_json;
    dbClient.insert( "test", insertObject );

    JsonArray array = JSON_EMPTY_ARR;
    array.push_back( JSON( "name"
                           << "item1" ) );
    array.push_back( JSON( "name"
                           << "item2" ) );
    dbClient.update( "test",
                     JSON( "value"
                           << "12345" ),
                     JSON( "$set" << JSON( "array" << array ) ),
                     false,
                     true );

    JsonObject result = dbClient.findOne( "test",
                                          JSON( "value"
                                                << "12345" ),
                                          nullptr );
    JsonObject expectedObject = R"({
                "_id": "1",
                "value": "12345",
                "array": [ { "name": "item1" }, { "name": "item2" }],
                "value2": null
             })"_json;
    EXPECT_EQ( result, expectedObject );
}

TYPED_TEST( IDbClientTestSuite, testAppendToArray /*2.16*/ ) {
    auto& dbClient = this->getDbClient();

    const auto insertObject = R"({
                "_id": "1",
                "array": []
             })"_json;
    const auto query = JSON( db::ID_FIELD_NAME << "1" );

    dbClient.insert( "test", insertObject );

    dbClient.update( "test",
                     query,
                     JSON( "$push" << JSON( "array" << JSON( "value"
                                                             << "1" ) ) ),
                     false,
                     true );
    {
        auto result = dbClient.findOne( "test", query, nullptr );
        const auto etalonObject = R"({
                "_id": "1",
                "array": [{"value" : "1"}]
             })"_json;

        EXPECT_EQ( result, etalonObject );
    }

    dbClient.update( "test",
                     query,
                     JSON( "$push" << JSON( "array" << JSON( "value"
                                                             << "2" ) ) ),
                     false,
                     true );
    {
        auto result = dbClient.findOne( "test", query, nullptr );
        const auto etalonObject = R"({
                "_id": "1",
                "array": [{"value" : "1"}, {"value" : "2"}]
             })"_json;
        EXPECT_EQ( result, etalonObject );
    }
}

TYPED_TEST( IDbClientTestSuite, testFindOneWithArrayField /*2.17*/ ) {
    auto& dbClient = this->getDbClient();

    const auto insertObject = R"({
                "_id": "1",
                "permissions": [ "zwave", "storage" ]
             })"_json;
    dbClient.insert( "test", insertObject );

    const auto queryJson = JSON( db::ID_FIELD_NAME << "1"
                                                   << "permissions" << JSON( "$in" << JSON_ARRAY_( "zwave" ) ) );
    const auto result = dbClient.findOne( "test", queryJson, nullptr );

    EXPECT_EQ( result, insertObject );
}

TYPED_TEST( IDbClientTestSuite, testComplexUpdate /*2.18*/ ) {
    auto& dbClient = this->getDbClient();

    const auto insertObject = R"({
                "_id": "2810",
                "key": "value",
                "obj": { "key": 2810 }
             })"_json;

    dbClient.insert( "test", insertObject );

    const auto json = JSON( "$set" << JSON( "key"
                                            << "S"
                                            << "obj" << JSON( "key" << 1028 ) ) );
    dbClient.update( "test", JSON( db::ID_FIELD_NAME << "2810" ), json, false, false );

    const auto result = dbClient.findOne( "test", JSON( db::ID_FIELD_NAME << "2810" ), nullptr );

    const auto expectedObject = R"({
            "_id": "2810",
            "key": "S",
            "obj": { "key": 1028 }
         })"_json;

    EXPECT_EQ( result, expectedObject );
}

/** @brief dbClient.has() should call a lambda passed to forEachNth() only once;
 * findAll() should properly limit result set size. */
TYPED_TEST( IDbClientTestSuite, testForEachNth_nToReturn /* 2.19 */ ) {
    auto& dbClient = this->getDbClient();
    const char* dbName = "testForEachNth_nToReturn";
    const int countKeys = 10;

    // put 10 records, unique id, isOdd: boolean
    for( int i = 0; i < countKeys; i++ ) {
        dbClient.insert( dbName,
                         JSON( db::ID_FIELD_NAME << std::to_string( i ) << "isOdd" << static_cast< bool >( i % 2 ) ) );
    }

    Json queryAllOdd = JSON( "isOdd" << true );
    int allOddCounter = 0;

    for( size_t limitRecords = 1; limitRecords <= countKeys / 2; limitRecords++ ) {
        allOddCounter = 0;
        dbClient.forEachNth( dbName,
                             queryAllOdd,
                             [&allOddCounter]( const Json& value, size_t number ) { allOddCounter++; },
                             limitRecords,
                             0 );
        EXPECT_EQ( allOddCounter, limitRecords )
            << "Try N: " << limitRecords << ": Lambda should be called for every odd number";
    }
}

}  // namespace
}  // namespace UT
}  // namespace HubPlatform
