#include "service_interfaces/reporter.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

namespace UT {

class ReporterTestSuite : public ::testing::Test {
protected:
    Reporter m_reporter{ "test" };
};

TEST_F( ReporterTestSuite, getCPUTime ) {
    JsonObject memUsage = m_reporter.getCPUTime();
    EXPECT_EQ( typeid( uint64_t ), typeid( m_reporter.getCPUTime() ) );
}

TEST_F( ReporterTestSuite, getProcessMemory ) {
    JsonObject memUsage = m_reporter.getProcessMemory();
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmRSS" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "RssAnon" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmPeak" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmLck" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmPin" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmHWM" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "RssFile" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "RssShmem" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmData" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmStk" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmExe" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmLib" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmPTE" ) );
    EXPECT_NO_THROW( jsonCheckInt( memUsage, "VmSwap" ) );
}

TEST_F( ReporterTestSuite, prepareReportData ) {
    JsonObject reportData;
    EXPECT_TRUE( parseJson( m_reporter.prepareReportData(), reportData ) );
    JsonObject data;
    EXPECT_TRUE( json_get( reportData, { "service", "test", "report_date" }, data ) );
    EXPECT_TRUE( json_get( reportData, { "service", "test", "service_status" }, data ) );
    EXPECT_TRUE( json_get( reportData, { "service", "test", "cpu_time" }, data ) );
    EXPECT_TRUE( json_get( reportData, { "service", "test", "mem_usage" }, data ) );
    EXPECT_TRUE( json_get( reportData, { "service", "test", "pid" }, data ) );
}

}  // namespace UT