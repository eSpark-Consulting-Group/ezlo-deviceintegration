#include "database_client/key_value_db_client.h"
#include "database_client/fake_db/fake_db_client.h"
#include "json_utils/json_wrapper.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <memory>

namespace HubPlatform {
namespace UT {

class KeyValueDbClientTest : public ::testing::Test {
protected:
    KeyValueDbClientTest()
    : m_dbClient( std::make_unique< FakeDbClient >() )
    , m_db( *m_dbClient, m_dbTableName, m_dbTableMaxSize ) {}

    void SetUp() override {
        cleanDb();
    }

    void TearDown() override {
        cleanDb();
    }

    IDbClient& getDBClient() {
        return *m_dbClient;
    }

    Db::KeyValueDbClient& getTypedDB() {
        return m_db;
    }

private:
    void cleanDb() {
        getDBClient().clean();
    }

protected:
    const std::size_t m_dbTableMaxSize = 4096;
    const std::string m_dbTableName = "test";

private:
    std::unique_ptr< IDbClient > m_dbClient;
    Db::KeyValueDbClient m_db;
};

TEST_F( KeyValueDbClientTest, hitDbTableLimit ) {
    for( auto i = 0; i < 52; i++ )
        getTypedDB().setJson( "key_" + std::to_string( i ),
                              JSON( "some_huge_key"
                                    << "some_huge_value" ) );
    EXPECT_EQ( getTypedDB().setJson( "key_53",
                                     JSON( "some_huge_key"
                                           << "some_huge_value" ) ),
               false );
}

TEST_F( KeyValueDbClientTest, setBool ) {
    ASSERT_EQ( getTypedDB().setBool( "bool_key", true ), true );
    EXPECT_EQ( getTypedDB().getBool( "bool_key" ).first, true );
    EXPECT_EQ( getDBClient().count( m_dbTableName, JSON_EMPTY_OBJ ), 1 );
}

TEST_F( KeyValueDbClientTest, setNumber ) {
    ASSERT_EQ( getTypedDB().setNumber( "number_key", 2810 ), true );
    EXPECT_EQ( getTypedDB().getNumber( "number_key" ).first, 2810 );
    EXPECT_EQ( getDBClient().count( m_dbTableName, JSON_EMPTY_OBJ ), 1 );
}

TEST_F( KeyValueDbClientTest, setString ) {
    ASSERT_EQ( getTypedDB().setString( "string_key", "2810" ), true );
    EXPECT_EQ( getTypedDB().getString( "string_key" ).first, "2810" );
    EXPECT_EQ( getDBClient().count( m_dbTableName, JSON_EMPTY_OBJ ), 1 );
}

TEST_F( KeyValueDbClientTest, setJson ) {
    ASSERT_EQ( getTypedDB().setJson( "key",
                                     JSON( "bd"
                                           << "2810" ) ),
               true );
    EXPECT_EQ( getTypedDB().getJson( "key" ).first,
               JSON( "bd"
                     << "2810" ) );
    EXPECT_EQ( getDBClient().count( m_dbTableName, JSON_EMPTY_OBJ ), 1 );
}

TEST_F( KeyValueDbClientTest, getBool ) {
    EXPECT_EQ( getTypedDB().getBool( "bool_key" ).second, false );
    EXPECT_NE( getTypedDB().getBool( "bool_key" ).first, true );
}

TEST_F( KeyValueDbClientTest, getNumber ) {
    EXPECT_EQ( getTypedDB().getNumber( "number_key" ).second, false );
}

TEST_F( KeyValueDbClientTest, getString ) {
    EXPECT_EQ( getTypedDB().getString( "string_key" ).second, false );
}

TEST_F( KeyValueDbClientTest, getJson ) {
    EXPECT_EQ( getTypedDB().getJson( "json_key" ).second, false );
}

TEST_F( KeyValueDbClientTest, exist ) {
    ASSERT_EQ( getTypedDB().setBool( "bool_key", true ), true );
    EXPECT_EQ( getTypedDB().exist( "bool_key" ), true );

    getTypedDB().remove( "bool_key" );
    EXPECT_EQ( getTypedDB().exist( "bool_key" ), false );
}

TEST_F( KeyValueDbClientTest, remove ) {
    ASSERT_EQ( getTypedDB().setString( "string_key", "2810" ), true );

    getTypedDB().remove( "bool_key" );
    EXPECT_EQ( getTypedDB().exist( "string_key" ), true );

    getTypedDB().remove( "string_key" );
    EXPECT_EQ( getTypedDB().exist( "string_key" ), false );
}

TEST_F( KeyValueDbClientTest, clear ) {
    const std::string key1{ "key_1" };
    const std::string key2{ "key_2" };

    ASSERT_TRUE( getTypedDB().setBool( key1, true ) );
    ASSERT_TRUE( getTypedDB().setBool( key2, false ) );

    getTypedDB().clear();

    EXPECT_FALSE( getTypedDB().exist( key1 ) );
    EXPECT_FALSE( getTypedDB().exist( key2 ) );
}

TEST_F( KeyValueDbClientTest, getInvalidJson ) {
    ASSERT_EQ( getTypedDB().setString( "key", "2810" ), true );
    EXPECT_EQ( getTypedDB().getJson( "key" ).first, Json::string_t( "2810" ) );
}

TEST_F( KeyValueDbClientTest, getInvalidString ) {
    ASSERT_EQ( getTypedDB().setNumber( "key", 2810 ), true );
    EXPECT_EQ( getTypedDB().getString( "key" ).second, false );
}

}  // namespace UT
}  // namespace HubPlatform
