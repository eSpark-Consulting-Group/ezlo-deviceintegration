#include "common_utils/hub_hardware_info.h"
#include "common_utils/formatters.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TESTPLAN

DONE    1. Time formatted string to nanoseconds convert test

*/

TEST( FormattersTestSuite, FormatedTimeStringToNanoseconds ) {
    uint64_t result;
    EXPECT_EQ( CommonUtils::timeFormatter( "2s", result ), true );
    EXPECT_EQ( result, 2000000000 );
    EXPECT_EQ( CommonUtils::timeFormatter( "2m", result ), true );
    EXPECT_EQ( result, 120000000000 );
    EXPECT_EQ( CommonUtils::timeFormatter( "2h", result ), true );
    EXPECT_EQ( result, 7200000000000 );

    EXPECT_EQ( CommonUtils::timeFormatter( "bug", result ), false );
}

}  // namespace UT
}  // namespace HubPlatform
