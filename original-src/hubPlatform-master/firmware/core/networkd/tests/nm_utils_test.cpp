#include "networkd/utils/nm_utils.h"

#include "log_utils/ezlo_log.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Mac address to string

*/

namespace HubPlatform {
namespace Network {

namespace UT {

class UtilsTestSuite : public ::testing::Test {
public:
    UtilsTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );
    }
};

TEST_F( UtilsTestSuite, macAddressToString ) {
    Utils::InterfaceInfo::MacAddress mac = { 0x00, 0x01, 0x02, 0x11, 0x22, 0xff };

    auto result = Utils::macAddrToString( mac );
    EXPECT_EQ( result, "00:01:02:11:22:ff" );
}

}  // namespace UT

}  // namespace Network
}  // namespace HubPlatform
