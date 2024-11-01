#include "service_metrics_data/hub_errors_table/hub_errors_table.h"

#include "database_client/idbclient.h"
#include "database_client/fake_db/fake_db_client.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <chrono>

/*

TESTPLAN

DONE    1. HubErrorsTable tests
DONE        1.1 Read from empty table
DONE        1.2 Update errors
DONE        1.3 Json conversions

*/

namespace HubPlatform {
namespace UT {

using namespace ::testing;

class HubErrorsTableTestSuite : public testing::Test {
public:
    void SetUp() override {
        m_dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >();
        m_dbClient->clean();
    }

    void TearDown() override {
        m_dbClient->clean();
        m_dbClient.reset();
    }

    IDbClient& getDBClient() {
        assert( m_dbClient );
        return *m_dbClient;
    }

private:
    std::unique_ptr< IDbClient > m_dbClient;
};

TEST_F( HubErrorsTableTestSuite, readFromEmptyTable ) /* 1.1 */ {
    using ErrorType = ServiceMetricsData::HubErrorsTable::ErrorType;

    ServiceMetricsData::HubErrorsTable table( getDBClient() );

    EXPECT_EQ( table.getAllErrors().size(), 0 );

    auto error = table.getError( ErrorType::KernelCrash );
    EXPECT_EQ( error.errorId, "kernel_crash" );
    EXPECT_EQ( error.count, 0 );
    EXPECT_EQ( error.lastUpdated, 0 );
}

TEST_F( HubErrorsTableTestSuite, updateErrors ) /* 1.2 */ {
    using ErrorType = ServiceMetricsData::HubErrorsTable::ErrorType;

    ServiceMetricsData::HubErrorsTable table( getDBClient() );
    EXPECT_EQ( table.getAllErrors().size(), 0 );

    table.incrementError( ErrorType::KernelCrash );
    EXPECT_EQ( table.getAllErrors().size(), 1 );

    const auto errorA1 = table.getError( ErrorType::KernelCrash );
    EXPECT_EQ( errorA1.errorId, "kernel_crash" );
    EXPECT_EQ( errorA1.count, 1 );
    EXPECT_GT( errorA1.lastUpdated, 0 );

    table.incrementError( ErrorType::UninterruptibleSleep );
    EXPECT_EQ( table.getAllErrors().size(), 2 );

    const auto errorB1 = table.getError( ErrorType::UninterruptibleSleep );
    EXPECT_EQ( errorB1.errorId, "uninterruptible_sleep" );
    EXPECT_EQ( errorB1.count, 1 );
    EXPECT_GT( errorB1.lastUpdated, 0 );

    std::this_thread::sleep_for( std::chrono::milliseconds( 1100 ) );

    table.incrementError( ErrorType::KernelCrash );
    table.incrementError( ErrorType::UninterruptibleSleep );

    EXPECT_EQ( table.getAllErrors().size(), 2 );

    const auto errorA2 = table.getError( ErrorType::KernelCrash );
    EXPECT_EQ( errorA2.errorId, errorA1.errorId );
    EXPECT_EQ( errorA2.count, 2 );
    EXPECT_GT( errorA2.lastUpdated, errorA1.lastUpdated );

    const auto errorB2 = table.getError( ErrorType::UninterruptibleSleep );
    EXPECT_EQ( errorB2.errorId, errorB1.errorId );
    EXPECT_EQ( errorB2.count, 2 );
    EXPECT_GT( errorB2.lastUpdated, errorB1.lastUpdated );

    std::map< std::string, ServiceMetricsData::HubErrorsTable::HubError > allErrors;
    for( auto& error : table.getAllErrors() ) {
        allErrors[ error.errorId ] = error;
    }

    {
        EXPECT_NO_THROW( allErrors.at( errorA2.errorId ) );
        auto& error = allErrors.at( errorA2.errorId );
        EXPECT_EQ( error.errorId, errorA2.errorId );
        EXPECT_EQ( error.count, 2 );
        EXPECT_EQ( error.lastUpdated, errorA2.lastUpdated );
    }

    {
        EXPECT_NO_THROW( allErrors.at( errorB2.errorId ) );
        auto& error = allErrors.at( errorB2.errorId );
        EXPECT_EQ( error.errorId, errorB2.errorId );
        EXPECT_EQ( error.count, 2 );
        EXPECT_EQ( error.lastUpdated, errorB2.lastUpdated );
    }
}

TEST_F( HubErrorsTableTestSuite, jsonConversions ) /* 1.3 */ {
    using HubError = ServiceMetricsData::HubErrorsTable::HubError;

    std::vector< HubError > errors;
    {
        HubError error;
        error.errorId = "error1";
        error.count = 1;
        error.lastUpdated = 10;
        errors.emplace_back( std::move( error ) );
    }

    {
        HubError error;
        error.errorId = "error2";
        error.count = 2;
        error.lastUpdated = 20;
        errors.emplace_back( std::move( error ) );
    }

    EXPECT_EQ( errors.size(), 2 );

    const auto jsonErrors = ServiceMetricsData::HubErrorsTable::toJsonArray( errors );
    EXPECT_EQ( jsonErrors.size(), 2 );

    const auto restoredErrors = ServiceMetricsData::HubErrorsTable::fromJsonArray( jsonErrors );
    EXPECT_EQ( restoredErrors.size(), 2 );

    ASSERT_EQ( restoredErrors.size(), errors.size() );
    for( size_t i = 0; i < errors.size(); ++i ) {
        auto& lhs = restoredErrors.at( i );
        auto& rhs = errors.at( i );
        EXPECT_EQ( lhs.errorId, rhs.errorId );
        EXPECT_EQ( lhs.count, rhs.count );
        EXPECT_EQ( lhs.lastUpdated, rhs.lastUpdated );
    }
}

}  // namespace UT
}  // namespace HubPlatform
