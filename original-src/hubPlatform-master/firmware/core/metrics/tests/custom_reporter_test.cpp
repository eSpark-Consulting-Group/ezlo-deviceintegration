#include "metrics/custom_reporter.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Metrics {
namespace UT {

class FakeCustomReporter : public CustomReporter {
public:
    explicit FakeCustomReporter( const std::string& processName );

    std::string prepareReportData() override;
};

std::string FakeCustomReporter::prepareReportData() {
    auto data = CustomReporter::prepareReportData();
    auto jsonData = Json::parse( data );
    jsonData[ "service" ][ "metrics_test" ][ "uptime" ] = 5;
    return jsonData.dump();
}

FakeCustomReporter::FakeCustomReporter( const std::string& processName )
: CustomReporter( processName ) {}

class CustomReporterTest : public ::testing::Test {
public:
    void SetUp() override;

protected:
    std::unique_ptr< CustomReporter > m_customReporter;
};

void CustomReporterTest::SetUp() {
    Test::SetUp();
    m_customReporter = std::make_unique< FakeCustomReporter >( "metrics_test" );
}

TEST_F( CustomReporterTest, ctor ) {
    try {
        CustomReporter customReporter{ "metrics_test" };
    }
    catch( const std::exception& ex ) {
        FAIL() << "CustomReporter ctor threw an exception: " << ex.what();
    }
}

TEST_F( CustomReporterTest, processStatus ) {
    const std::string processName = "metrics_test";  // This process name
    CustomReporter customReporter{ processName };

    std::string report;
    ASSERT_GT( CustomReporter::fetchPidByProcessName( processName ), 0 );

    ASSERT_NO_THROW( customReporter.getProcStatus() ) << "Should not throw on valid input";
}

TEST_F( CustomReporterTest, fetchPidByProcessName_nonExistingProcessStatusFails ) {
    const std::string nonExistingProcess = "metrics_test-11111111111111";
    CustomReporter customReporter{ nonExistingProcess };

    std::string report;

    ASSERT_EQ( CustomReporter::fetchPidByProcessName( nonExistingProcess ), 0 );

    std::string statusData;
    ASSERT_NO_THROW( statusData = customReporter.getProcStatus() ) << "Should not throw on valid input";

    ASSERT_TRUE( statusData.empty() );
}

TEST_F( CustomReporterTest, processUptime ) {
    const std::string processName = "metrics_test";  // This process name

    std::string report;
    ASSERT_GT( m_customReporter->fetchPidByProcessName( processName ), 0 );

    ASSERT_NO_THROW( m_customReporter->getProcStatus() ) << "Should not throw on valid input";

    auto processReport = m_customReporter->prepareReportData();
    auto jsonReport = Json::parse( processReport );
    auto data = jsonReport[ "service" ][ "metrics_test" ];
    EXPECT_TRUE( json_contains( data, "uptime" ) );
    EXPECT_EQ( json_get_int( data, "uptime" ), 5 );
}

}  // namespace UT
}  // namespace Metrics
}  // namespace HubPlatform
