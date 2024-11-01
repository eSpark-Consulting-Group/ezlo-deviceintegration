
#include "database_client/dbqueryparser.h"
#include "database_client/lmdb_client.h"
#include "database_client/dbtools.h"

#include "test_utils/temporary_folder_wrapper.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

namespace HubPlatform {
namespace UT {
namespace {

using namespace ::testing;

class LMDBClientTestSuite : public Test {
    constexpr static std::uint8_t UniquePrefixSize = 5;
    constexpr static std::uint8_t DelimiterSize = 1;

public:
    std::string getStorageFilePath() const {
        return m_folderWrapper.getPath() + "/" + StorageFileName;
    }

    std::string getTemporaryStorageFilePath() const {
        return m_folderWrapper.getPath() + "/" + TemporaryStorageFileName;
    }

public:
    void SetUp() override {
        m_client = createDbClient( getStorageFilePath(), getTemporaryStorageFilePath() );

        m_client->clean();
    }

    void TearDown() override {
        m_client->clean();
        m_client.reset();
    }

protected:
    Tests::TemporaryFolderWrapper m_folderWrapper;
    std::unique_ptr< IDbClient > m_client;
};

TEST_F( LMDBClientTestSuite, testTemporaryDb ) {
    const auto& tableNameForTempDB = LMDBClient::createTemporaryTableName( "test" );

    m_client->insert( tableNameForTempDB,
                      JSON( db::ID_FIELD_NAME << "id"
                                              << "value" << 10 ) );

    std::ifstream ifile( getTemporaryStorageFilePath() );
    EXPECT_EQ( true, static_cast< bool >( ifile ) );

    auto result = m_client->findAll( tableNameForTempDB, Json::object(), nullptr, 1, 0 );
    EXPECT_EQ( result.front(),
               JSON( db::ID_FIELD_NAME << "id"
                                       << "value" << 10 ) );
}

TEST_F( LMDBClientTestSuite, testFindOne ) {
    const auto& tableNameForTempDB = LMDBClient::createTemporaryTableName( "test" );
    m_client->insert( tableNameForTempDB, JSON( "value" << 1 ) );
    m_client->insert( tableNameForTempDB, JSON( "value" << 2 ) );
    m_client->insert( tableNameForTempDB, JSON( "value" << 3 ) );
    m_client->insert( tableNameForTempDB, JSON( "value" << 4 ) );
    m_client->insert( tableNameForTempDB, JSON( "value" << 5 ) );
    auto fieldsToReturn = JSON( "value" << 1 );
    std::vector< JsonObject > record = m_client->findAll( tableNameForTempDB, JSON_EMPTY_OBJ, &fieldsToReturn );
    std::vector< JsonObject > shouldBe = {
        JSON( "value" << 1 ), JSON( "value" << 2 ), JSON( "value" << 3 ), JSON( "value" << 4 ), JSON( "value" << 5 )
    };
    EXPECT_EQ( record, shouldBe );
    JsonObject record1 = m_client->findOne( tableNameForTempDB, JSON( "value" << 4 ), &fieldsToReturn );
    JsonObject shouldBe1 = JSON( "value" << 4 );
    EXPECT_EQ( record1.dump(), shouldBe1.dump() );
    record1 = m_client->findOne( tableNameForTempDB, JSON( "value" << 5 ), &fieldsToReturn );
    shouldBe1 = JSON( "value" << 5 );
    EXPECT_EQ( record1.dump(), shouldBe1.dump() );
}

TEST_F( LMDBClientTestSuite, testUpdate ) {
    const auto& tableNameForTempDB = LMDBClient::createTemporaryTableName( "test" );
    m_client->update( tableNameForTempDB, JSON( "value" << 1 ), JSON( "value" << 1 ), true, false );
    JsonObject fieldsToReturn = JSON( "value" << 1 );
    JsonObject record = m_client->findOne( tableNameForTempDB, JSON( "value" << 1 ), &fieldsToReturn );
    JsonObject shouldBe = JSON( "value" << 1 );
    EXPECT_EQ( record.dump(), shouldBe.dump() );
    m_client->update( tableNameForTempDB, JSON( "value" << 1 ), JSON( "value" << 1 << "value1" << 1 ), false, false );
    fieldsToReturn = JSON( "value" << 1 << "value1" << 1 );
    record = m_client->findOne( tableNameForTempDB, JSON( "value" << 1 ), &fieldsToReturn );
    shouldBe = JSON( "value" << 1 << "value1" << 1 );
    EXPECT_EQ( record.dump(), shouldBe.dump() );
}

TEST_F( LMDBClientTestSuite, testRemove ) {
    const auto& tableNameForTempDB1 = LMDBClient::createTemporaryTableName( "test1" );
    const auto& tableNameForTempDB2 = LMDBClient::createTemporaryTableName( "test2" );
    m_client->insert( tableNameForTempDB1, JSON( "value" << 1 ) );
    m_client->insert( tableNameForTempDB2, JSON( "value" << 1 ) );
    m_client->remove( tableNameForTempDB2, JSON_EMPTY_OBJ, false, false );
    JsonObject fieldsToReturn = JSON( "value" << 1 );
    JsonObject record = m_client->findOne( tableNameForTempDB1, JSON( "value" << 1 ), &fieldsToReturn );
    JsonObject shouldBe = JSON( "value" << 1 );
    EXPECT_EQ( record.dump(), shouldBe.dump() );
    record = m_client->findOne( tableNameForTempDB2, JSON( "value" << 1 ), nullptr );
    shouldBe = JSON_EMPTY_OBJ;
    EXPECT_EQ( record.dump(), shouldBe.dump() );
}

TEST_F( LMDBClientTestSuite, testSet ) {
    const auto& tableNameForTempDB1 = LMDBClient::createTemporaryTableName( "test1" );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "123"
                            << "value"
                            << "1"
                            << "key"
                            << "1" ) );
    m_client->update( tableNameForTempDB1,
                      JSON( "_id"
                            << "123" ),
                      JSON( "$set" << JSON( "value"
                                            << "2"
                                            << "key"
                                            << "2" ) ) );
    JsonObject fieldsToReturn = JSON( "value" << 1 << "key" << 1 );
    JsonObject record = m_client->findOne( tableNameForTempDB1,
                                           JSON( "_id"
                                                 << "123" ),
                                           &fieldsToReturn );
    JsonObject shouldBe = JSON( "value"
                                << "2"
                                << "key"
                                << "2" );
    EXPECT_EQ( record.dump(), shouldBe.dump() );

    m_client->update( tableNameForTempDB1,
                      JSON( "_id"
                            << "123" ),
                      JSON( "$set" << JSON( "_id"
                                            << "123"
                                            << "value"
                                            << "3"
                                            << "key"
                                            << "3" ) ) );
    record = m_client->findOne( tableNameForTempDB1,
                                JSON( "_id"
                                      << "123" ),
                                &fieldsToReturn );
    shouldBe = JSON( "value"
                     << "3"
                     << "key"
                     << "3" );
    EXPECT_EQ( record.dump(), shouldBe.dump() );
}

TEST_F( LMDBClientTestSuite, testIn ) {
    const auto& tableNameForTempDB1 = LMDBClient::createTemporaryTableName( "test1" );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "1"
                            << "value"
                            << "1" ) );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "2"
                            << "value"
                            << "1" ) );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "3"
                            << "value"
                            << "2" ) );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "4"
                            << "value"
                            << "2" ) );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "5"
                            << "value"
                            << "3" ) );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "6"
                            << "value"
                            << "3" ) );
    auto result =
        m_client->findAll( tableNameForTempDB1, JSON( "value" << JSON( "$in" << JSON_ARRAY_( "1" ) ) ), nullptr );
    EXPECT_EQ( result.size(), 2 );
    EXPECT_EQ( result[ 0 ].dump(),
               JSON( "_id"
                     << "1"
                     << "value"
                     << "1" )
                   .dump() );
    EXPECT_EQ( result[ 1 ].dump(),
               JSON( "_id"
                     << "2"
                     << "value"
                     << "1" )
                   .dump() );
    result =
        m_client->findAll( tableNameForTempDB1, JSON( "value" << JSON( "$in" << JSON_ARRAY_( "1", "3" ) ) ), nullptr );
    EXPECT_EQ( result.size(), 4 );
    EXPECT_EQ( result[ 0 ].dump(),
               JSON( "_id"
                     << "1"
                     << "value"
                     << "1" )
                   .dump() );
    EXPECT_EQ( result[ 1 ].dump(),
               JSON( "_id"
                     << "2"
                     << "value"
                     << "1" )
                   .dump() );
    EXPECT_EQ( result[ 2 ].dump(),
               JSON( "_id"
                     << "5"
                     << "value"
                     << "3" )
                   .dump() );
    EXPECT_EQ( result[ 3 ].dump(),
               JSON( "_id"
                     << "6"
                     << "value"
                     << "3" )
                   .dump() );
}

TEST_F( LMDBClientTestSuite, testPull ) {
    const auto& tableNameForTempDB1 = LMDBClient::createTemporaryTableName( "test1" );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "1"
                            << "value"
                            << JSON_ARRAY_( JSON( "key1"
                                                  << "1" ),
                                            JSON( "key2"
                                                  << "2" ) ) ) );
    m_client->update( tableNameForTempDB1,
                      JSON( "_id"
                            << "1" ),
                      JSON( "$pull" << JSON( "value" << JSON( "key1"
                                                              << "1" ) ) ) );
    m_client->remove( tableNameForTempDB1, JSON( "value" << JSON_EMPTY_ARR ) );
    JsonObject fieldsToReturn = JSON( "value" << 1 );
    auto result = m_client->findOne( tableNameForTempDB1,
                                     JSON( "_id"
                                           << "1" ),
                                     &fieldsToReturn );
    EXPECT_EQ( result.dump(),
               JSON( "value" << JSON_ARRAY_( JSON( "key2"
                                                   << "2" ) ) )
                   .dump() );
    result = m_client->findOne( tableNameForTempDB1,
                                JSON( "value" << JSON_ARRAY_( JSON( "key2"
                                                                    << "2" ) ) ),
                                &fieldsToReturn );
    m_client->update( tableNameForTempDB1,
                      JSON( "_id"
                            << "1" ),
                      JSON( "$pull" << JSON( "value" << JSON( "key2"
                                                              << "2" ) ) ) );
    result = m_client->findOne( tableNameForTempDB1,
                                JSON( "_id"
                                      << "1" ),
                                &fieldsToReturn );
    EXPECT_EQ( result.dump(), JSON( "value" << JSON_EMPTY_ARR ).dump() );
    result = m_client->findOne( tableNameForTempDB1, JSON( "value" << JSON_EMPTY_ARR ), &fieldsToReturn );
    EXPECT_FALSE( result.empty() );
    m_client->remove( tableNameForTempDB1, JSON( "value" << JSON_EMPTY_ARR ) );
    result = m_client->findOne( tableNameForTempDB1,
                                JSON( "_id"
                                      << "1" ),
                                &fieldsToReturn );
    EXPECT_TRUE( result.empty() );
}

TEST_F( LMDBClientTestSuite, testExist ) {
    const auto& tableNameForTempDB1 = LMDBClient::createTemporaryTableName( "test1" );
    m_client->insert( tableNameForTempDB1,
                      JSON( "_id"
                            << "1"
                            << "value"
                            << "1" ) );
    auto result = m_client->findOne( tableNameForTempDB1, JSON( "value" << JSON( "$exists" << true ) ), nullptr );
    EXPECT_FALSE( result.empty() );
    m_client->update( tableNameForTempDB1,
                      JSON( "_id"
                            << "1" ),
                      JSON( "value" << nullptr ) );
    result = m_client->findOne( tableNameForTempDB1, JSON( "value" << JSON( "$exists" << true ) ), nullptr );
    EXPECT_FALSE( result.empty() );
}

TEST_F( LMDBClientTestSuite, testFixBrokenPrimaryKeys ) {
    const auto& dbName = LMDBClient::createTemporaryTableName( "test" );
    m_client->insert( dbName,
                      JSON( "_id"
                            << "1"
                            << "value"
                            << "1" ) );

    m_client->insert( dbName,
                      JSON( "_id"
                            << "2"
                            << "value"
                            << "2" ) );

    m_client->insert( dbName,
                      JSON( "_id"
                            << "3"
                            << "value"
                            << "3" ) );

    m_client->insert( dbName,
                      JSON( "_id"
                            << "4"
                            << "value"
                            << "4" ) );

    m_client->update( dbName,
                      JSON( "_id"
                            << "2" ),
                      []( Json& value ) { value[ "_id" ] = "_2"; } );

    m_client->update( dbName,
                      JSON( "_id"
                            << "4" ),
                      []( Json& value ) { value[ "_id" ] = "_4"; } );

    m_client->fixBrokenPrimaryKeys( dbName );

    EXPECT_EQ( m_client
                   ->findAll( dbName,
                              JSON( "_id"
                                    << "1" ),
                              nullptr )
                   .size(),
               1 );
    EXPECT_EQ( m_client
                   ->findAll( dbName,
                              JSON( "_id"
                                    << "3" ),
                              nullptr )
                   .size(),
               1 );
    EXPECT_EQ( m_client
                   ->findAll( dbName,
                              JSON( "_id"
                                    << "_2" ),
                              nullptr )
                   .size(),
               1 );
    EXPECT_EQ( m_client
                   ->findAll( dbName,
                              JSON( "_id"
                                    << "_4" ),
                              nullptr )
                   .size(),
               1 );

    EXPECT_EQ( m_client
                   ->findOne( dbName,
                              JSON( "_id"
                                    << "1" ),
                              nullptr )
                   .dump(),
               JSON( "_id"
                     << "1"
                     << "value"
                     << "1" )
                   .dump() );
    EXPECT_EQ( m_client
                   ->findOne( dbName,
                              JSON( "_id"
                                    << "3" ),
                              nullptr )
                   .dump(),
               JSON( "_id"
                     << "3"
                     << "value"
                     << "3" )
                   .dump() );
    EXPECT_EQ( m_client
                   ->findOne( dbName,
                              JSON( "_id"
                                    << "_2" ),
                              nullptr )
                   .dump(),
               JSON( "_id"
                     << "_2"
                     << "value"
                     << "2" )
                   .dump() );
    EXPECT_EQ( m_client
                   ->findOne( dbName,
                              JSON( "_id"
                                    << "_4" ),
                              nullptr )
                   .dump(),
               JSON( "_id"
                     << "_4"
                     << "value"
                     << "4" )
                   .dump() );

    EXPECT_EQ( m_client
                   ->findAll( dbName,
                              JSON( "_id"
                                    << "2" ),
                              nullptr )
                   .size(),
               0 );
    EXPECT_EQ( m_client
                   ->findAll( dbName,
                              JSON( "_id"
                                    << "4" ),
                              nullptr )
                   .size(),
               0 );
}

TEST_F( LMDBClientTestSuite, testPrimaryKeys ) {
    const auto& dbName = LMDBClient::createTemporaryTableName( "test1" );
    m_client->insert( dbName,
                      JSON( "_id"
                            << "1"
                            << "value"
                            << "1" ) );

    m_client->insert( dbName,
                      JSON( "_id"
                            << "2"
                            << "value"
                            << "2" ) );

    m_client->insert( dbName,
                      JSON( "_id"
                            << "100"
                            << "value"
                            << "100" ) );

    auto keys = m_client->getPrimaryKeys( dbName );
    std::sort( keys.begin(), keys.end() );

    EXPECT_THAT( keys, ::testing::ElementsAre( "1", "100", "2" ) );
}

}  // namespace
}  // namespace UT
}  // namespace HubPlatform
