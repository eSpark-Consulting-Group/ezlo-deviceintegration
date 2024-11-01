#include "networkd/monitoring/nm_monitor_utils.h"
#include "networkd/scan/nm_scan_utils.h"

#include "networkd/connection_service/nm_connection_defs.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Filter scanned networks

*/

namespace HubPlatform {
namespace Network {

namespace UT {

class ScanUtilsTestSuite : public ::testing::Test {
public:
    ScanUtilsTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );
    }
};

TEST_F( ScanUtilsTestSuite, filterScannedNetworks ) {
    auto addToScan = []( Json& scan, std::string ssid, std::string bssid, int signalLevel ) {
        auto& list = scan[ "networks" ];
        list.push_back( JSON( "ssid" << ssid << "bssid" << bssid << "signalLevel" << signalLevel << "channel" << 11
                                     << "security"
                                     << "psk2" ) );
    };

    {  // empty scan
        auto scan = JSON( "networks" << JSON_EMPTY_ARR );
        auto result = getScannedNetwork( scan, "ssid", "bssid" );
        EXPECT_FALSE( result.isValid() );
    }

    {  // filter by ssid: find strongest signal
        JsonObject scan;
        addToScan( scan, "ssid-A", "bssid-A", 10 );
        addToScan( scan, "ssid-B", "bssid-B", 10 );
        addToScan( scan, "ssid-B", "bssid-BB", 20 );
        addToScan( scan, "ssid-C", "bssid-C", 10 );
        auto result = getScannedNetwork( scan, "ssid-B" );
        EXPECT_TRUE( result.isValid() );
        EXPECT_EQ( result.m_ssid, "ssid-B" );
        EXPECT_EQ( result.m_bssid, "bssid-BB" );
        EXPECT_EQ( result.m_signalPercentLevel, 20 );
    }

    {  // filter by ssid and bssid: find exact network
        JsonObject scan;
        addToScan( scan, "ssid-A", "bssid-A", 10 );
        addToScan( scan, "ssid-B", "bssid-B", 10 );
        addToScan( scan, "ssid-B", "bssid-BB", 20 );
        addToScan( scan, "ssid-B", "bssid-BBB", 30 );
        addToScan( scan, "ssid-C", "bssid-C", 10 );
        auto result = getScannedNetwork( scan, "ssid-B", "bssid-BB" );
        EXPECT_TRUE( result.isValid() );
        EXPECT_EQ( result.m_ssid, "ssid-B" );
        EXPECT_EQ( result.m_bssid, "bssid-BB" );
        EXPECT_EQ( result.m_signalPercentLevel, 20 );
    }

    {  // filter by ssid and non-existing bssid: find strongest signal
        JsonObject scan;
        addToScan( scan, "ssid-A", "bssid-A", 10 );
        addToScan( scan, "ssid-B", "bssid-B", 10 );
        addToScan( scan, "ssid-B", "bssid-BB", 20 );
        addToScan( scan, "ssid-C", "bssid-C", 10 );
        auto result = getScannedNetwork( scan, "ssid-B", "bssid-foo" );
        EXPECT_TRUE( result.isValid() );
        EXPECT_EQ( result.m_ssid, "ssid-B" );
        EXPECT_EQ( result.m_bssid, "bssid-BB" );
        EXPECT_EQ( result.m_signalPercentLevel, 20 );
    }
}

}  // namespace UT

}  // namespace Network
}  // namespace HubPlatform
