// Scan tests for openwrt iw scanner
#include "networkd/monitoring/nm_monitor_utils.h"
#include "networkd/scan/nm_scan_utils.h"
#include "networkd/scan/nm_wifi_scan_result_parser.h"
#include "networkd/connection_service/nm_connection_defs.h"
#include "json_utils/json_utils.h"
#include "networkd/tests/resources/iw_scan_resources.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Convert wifi signal dbm to percent
DONE    2. Parse OpenWrt virtual interface status
DONE    3. Parse G150 virtual interface status
DONE    4. Parse iwpriv wifi scan result
DONE    5. Parse iw wifi scan result
DONE    6. Parse IP CIDR

*/

namespace HubPlatform {
namespace Network {

namespace UT {

class IwParseTests : public ::testing::Test {
public:
    IwParseTests() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::INFO );
    }

    Json networkMapToJson( const NetworkMap& networkMap ) {
        Json result;
        auto& jsonList = result[ std::string( WifiListKey ) ] = Json::array();
        for( auto& elem : networkMap ) {
            jsonList.push_back( elem.second.toJson( false ) );
        }

        return result;
    }

    Json parseIwWifiScanResult( const std::string& scanResult ) {
        IwScanResultParser parser;
        parser.parse( scanResult );
        return networkMapToJson( parser.getScannedItems() );
    }

    static Json getSortedWifiScanResult( Json json ) {
        const auto networksIt = json.find( "networks" );
        if( networksIt != json.end() ) {
            std::sort( networksIt.value().begin(), networksIt.value().end(), []( const Json& a, const Json& b ) {
                return json_get_str( a, "ssid" ) < json_get_str( b, "ssid" );
            } );
        }

        return json;
    }
};

TEST_F( IwParseTests, parseIwWifiScanResult ) {
    EXPECT_EQ( getSortedWifiScanResult( parseIwWifiScanResult( Resources::iwScanResults ) ).dump( 4 ),
               getSortedWifiScanResult( Json::parse( Resources::expectedNetworks ) ).dump( 4 ) );
}

TEST_F( IwParseTests, sameSsidTwoBandsNetwork ) {
    IwScanResultParser parser;
    // TODO: Add extended flag: add extended fields
    parser.parse( std::string( Resources::SameSsidNetwork1Associated ) + std::string( "\n" ) +
                  Resources::SameSsidNetwork2 );

    NetworkMap networkMap = parser.getScannedItems();
    auto& networksList = parser.getScannedItemsList();

    EXPECT_EQ( networkMap.size(), 1 );
    EXPECT_EQ( networksList.size(), 2 );
}

TEST_F( IwParseTests, network5GhzAssociated ) {
    IwScanResultParser parser;
    parser.parse( std::string( Resources::SameSsidNetwork1Associated ) );

    NetworkMap networkMap = parser.getScannedItems();
    auto& networksList = parser.getScannedItemsList();

    EXPECT_EQ( networkMap.size(), 1 );
    EXPECT_EQ( networksList.size(), 1 );

    NetworkItem expectedNetwork;
    expectedNetwork.m_aux.m_associated = true;
    expectedNetwork.m_aux.m_band = NetworkItemAuxFields::Band5Ghz;
    expectedNetwork.m_aux.m_frequency = 5180;
    expectedNetwork.m_aux.m_signal = -51.00;
    expectedNetwork.m_aux.m_capabilitiesFlags = 0x1111;

    auto& scannedNetwork = networksList.at( 0 );
    EXPECT_EQ( scannedNetwork.m_aux.m_associated, expectedNetwork.m_aux.m_associated ) << "Associated field";
    EXPECT_EQ( scannedNetwork.m_aux.m_frequency, expectedNetwork.m_aux.m_frequency ) << "Frequencies";
    EXPECT_EQ( scannedNetwork.m_aux.m_band, expectedNetwork.m_aux.m_band ) << "Bands";
    EXPECT_FLOAT_EQ( scannedNetwork.m_aux.m_signal, expectedNetwork.m_aux.m_signal ) << "Signal";
    EXPECT_EQ( scannedNetwork.m_aux.m_capabilitiesFlags, expectedNetwork.m_aux.m_capabilitiesFlags )
        << "network capabilities";
}

TEST_F( IwParseTests, network24Ghz ) {
    IwScanResultParser parser;
    parser.parse( std::string( Resources::SameSsidNetwork2 ) );

    NetworkMap networkMap = parser.getScannedItems();
    auto& networksList = parser.getScannedItemsList();

    EXPECT_EQ( networkMap.size(), 1 );
    EXPECT_EQ( networksList.size(), 1 );

    NetworkItem expectedNetwork;
    expectedNetwork.m_aux.m_associated = false;
    expectedNetwork.m_aux.m_band = NetworkItemAuxFields::Band2_4Ghz;
    expectedNetwork.m_aux.m_frequency = 2462;
    expectedNetwork.m_aux.m_signal = -53.00;
    expectedNetwork.m_aux.m_capabilitiesFlags = 0x1411;

    auto& scannedNetwork = networksList.at( 0 );
    EXPECT_EQ( scannedNetwork.m_aux.m_associated, expectedNetwork.m_aux.m_associated ) << "Associated field";
    EXPECT_EQ( scannedNetwork.m_aux.m_frequency, expectedNetwork.m_aux.m_frequency ) << "Frequencies";
    EXPECT_EQ( scannedNetwork.m_aux.m_band, expectedNetwork.m_aux.m_band ) << "Bands";
    EXPECT_FLOAT_EQ( scannedNetwork.m_aux.m_signal, expectedNetwork.m_aux.m_signal ) << "Signal";
    EXPECT_EQ( scannedNetwork.m_aux.m_capabilitiesFlags, expectedNetwork.m_aux.m_capabilitiesFlags )
        << "network capabilities";
}

}  // namespace UT
}  // namespace Network
}  // namespace HubPlatform
