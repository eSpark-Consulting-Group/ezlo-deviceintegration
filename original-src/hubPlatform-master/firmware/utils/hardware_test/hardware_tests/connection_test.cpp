#include "hardware_test/hardware_tests/connection_test.h"

#include "hardware_test/utils/test_log.h"

#include "base_utils/timeutils.h"
#include "base_utils/utils.h"
#include "json_utils/json_utils.h"

#include <iomanip>
#include <fstream>

#define __TEST_WIFI_INTERFACE__
#define WIFI_SCAN_RESULTS_LIMIT 3

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "Connection"
constexpr std::chrono::seconds TEST_TIMEOUT{ 30 };

namespace {
TestLog testLog( TEST_NAME );
}  // namespace

ConnectionTest::ConnectionTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef )
, m_exitSignal( exitSignalRef ) {}

HardwareTestInterface::TestResult ConnectionTest::run() {
    TestResult testResult = HardwareTestInterface::TestResult::DONE;
    TestResult subTestResult = HardwareTestInterface::TestResult::UNKNOWN;

    static const std::string startingTestMessage = std::string( "Starting " ).append( m_name ).append( " test ..." );
    JsonArray jSubTestsResults;
    std::string subTestName;

    testLog() << startingTestMessage;
    time_t startTestTime = TimeUtils::getMonotonicTime();
    time_t startSubTestTime = TimeUtils::getMonotonicTime();

    // list supported interfaces
    auto supportedInterfaces = NetworkInterface::getSupportedInterfaces();

    {
        std::stringstream interfaces;
        interfaces << "Supported interfaces:";
        for( const auto& interfaceName : supportedInterfaces ) {
            interfaces << " " << interfaceName;
        }
        testLog() << interfaces.str();
    }

    // getifaddrs uses dynamic allocation - use freeifaddrs to de-allocate at the end
    struct ifaddrs* pInterfaceAddress = nullptr;
    getifaddrs( &pInterfaceAddress );
    std::unordered_set< NetworkInterface::NetworkInterfaceUPtr > interfaceAddresses;
    for( struct ifaddrs* ifa = pInterfaceAddress; ifa != NULL; ifa = ifa->ifa_next ) {
        if( ifa->ifa_addr->sa_family == AF_INET ) {
            // a valid IPv4 address
            auto interface = NetworkInterface::createInterface( ifa );
            if( interface != nullptr ) {
                interfaceAddresses.emplace( std::move( interface ) );
            }
        }
        else if( ifa->ifa_addr->sa_family == AF_INET6 ) {
            // Check it is a valid IPv6 address - list IPv4 only
        }
    }

    subTestResult = testPing( jSubTestsResults, interfaceAddresses );
    if( subTestResult == TestResult::FAILED ) {
        testResult = TestResult::FAILED;
    }
    subTestResult = testWiFi( jSubTestsResults, supportedInterfaces );
    if( subTestResult == TestResult::FAILED ) {
        testResult = TestResult::FAILED;
    }

    testLog() << getTestResultReport( testResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    // restore interface states
    for( const auto& supportedInterface : supportedInterfaces ) {
        NetworkInterface::setInterfaceUp( supportedInterface, false );
        for( const auto& interfaceWithAddress : interfaceAddresses ) {
            if( supportedInterface == interfaceWithAddress->getName() && interfaceWithAddress->isUp() ) {
                NetworkInterface::setInterfaceUp( supportedInterface );
            }
        }
    }

    if( pInterfaceAddress != NULL )
        freeifaddrs( pInterfaceAddress );

    return testResult;
}

HardwareTestInterface::TestResult ConnectionTest::testPing(
    Json& jTestResults,
    std::unordered_set< NetworkInterface::NetworkInterfaceUPtr >& interfaceAddresses ) {
    static constexpr char const pingTestName[] = "Connection ( Ping )";
    const std::string subTestName = "Ping";
    static const std::string startingTestMessage = std::string( "Starting " ).append( pingTestName ).append( "..." );
    testLog() << startingTestMessage;

    auto startTestTime = TimeUtils::getMonotonicTime();
    TestResult testResult = TestResult::DONE;
    static int testsPassedValue = 0;
    static const int testsPassThreshold = 1;

    Json jSubTestsResults;
    for( const auto& item : interfaceAddresses ) {
        testLog() << std::setw( 8 ) << std::left << item->getName() << " address: " << std::setw( 16 )
                  << item->getAddress() << ( item->isUp() ? " UP" : " DOWN" ) << ( item->isWired() ? " WIRED" : "" )
                  << ( item->isWireless() ? " WIRELESS" : "" ) << ( item->isLoopback() ? " LOOPBACK" : "" );
        if( !item->isLoopback() && item->hasAddress() ) {
            auto startTestTime = TimeUtils::getMonotonicTime();
            testLog() << "Starting ping test for " << item->getName();
            const bool pingDone = item->doPingTest( m_pingAddr );
            if( pingDone ) {
                ++testsPassedValue;
            }
            jSubTestsResults.push_back(
                JSON( "name" << subTestName << "interface" << item->getName() << "result" << pingDone ) );
            testLog() << getTestResultReport( std::string( subTestName ).append( " " ).append( item->getName() ),
                                              ( pingDone ? TestResult::DONE : TestResult::FAILED ),
                                              TimeUtils::getDiffMonotonicTime( startTestTime ) );
        }
    }

    bool subTestPassed = ( testsPassedValue >= testsPassThreshold );

    if( !subTestPassed ) {
        testResult = TestResult::FAILED;
    }
    jTestResults[ "name" ] = pingTestName;
    jTestResults[ "result" ] = subTestPassed;
    jTestResults[ "threshold" ] = testsPassThreshold;
    jTestResults[ "value" ] = testsPassedValue;
    jTestResults[ "subtests" ] = jSubTestsResults;
    testLog() << getTestResultJsonReport( jTestResults );

    testLog() << getTestResultReport( pingTestName, testResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    return testResult;
}

HardwareTestInterface::TestResult ConnectionTest::testWiFi( Json& jTestResults,
                                                            std::vector< std::string >& supportedInterfaces ) {
    static constexpr char const wifiTestName[] = "Connection ( WiFi )";
    const std::string subTestName = "WiFi";
    static const std::string startingTestMessage = std::string( "Starting " ).append( wifiTestName ).append( "..." );

    TestResult testResult = TestResult::DONE;
    static int testsPassedValue = 0;
    static const int testsPassThreshold = 1;

#ifdef __TEST_WIFI_INTERFACE__
    testLog() << startingTestMessage;

    auto startTestTime = TimeUtils::getMonotonicTime();
    Json jSubTestsResults;
    for( const auto& item : supportedInterfaces ) {
        if( NetworkInterface::isWired( item ) ) {
            testLog() << "Setting interface " << item << " DONW...";
            NetworkInterface::setInterfaceUp( item, false );
        }
        else if( NetworkInterface::isWireless( item ) ) {
            auto startTestTime = TimeUtils::getMonotonicTime();

            testLog() << "Setting interface " << item << " UP...";
            NetworkInterface::setInterfaceUp( item );
            bool result = NetworkInterface::isUp( item );
            if( result ) {
                auto jNetworks = scanWiFiNetworks( item );
                if( jNetworks.size() > 0 ) {
                    result = true;
                    jSubTestsResults.push_back(
                        JSON( "name" << subTestName << "interface" << item << "result" << jNetworks ) );
                    ++testsPassedValue;
                }
                else {
                    result = false;
                }
            }
            testLog() << getTestResultReport( std::string( subTestName ).append( " " ).append( item ),
                                              ( result ? TestResult::DONE : TestResult::FAILED ) );
        }
    }
    auto subTestPassed = ( testsPassedValue >= testsPassThreshold );

    if( !subTestPassed ) {
        testResult = TestResult::FAILED;
    }
    jTestResults[ "name" ] = wifiTestName;
    jTestResults[ "result" ] = subTestPassed;
    jTestResults[ "threshold" ] = testsPassThreshold;
    jTestResults[ "value" ] = testsPassedValue;
    jTestResults[ "subtests" ] = jSubTestsResults;
    testLog() << jTestResults;
    testLog() << getTestResultJsonReport( jTestResults );

    testLog() << getTestResultReport( wifiTestName, testResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );
#endif

    return testResult;
}

JsonArray ConnectionTest::scanWiFiNetworks( const std::string& ifaceName ) {
    JsonArray jScanResults;

    testLog() << "Getting wireless networks nearby ( " << ifaceName << " )...";
    const std::string cmd = "iw dev " + ifaceName +
                            " scan | grep -o -e \"SSID: .*\" -e \"freq: [0-9]*\" -e \"signal: .*\" | awk -v "
                            "RS='[,\\n]' '{a=$0;getline b; getline c; print c,b,a}' OFS=,";
    std::istringstream pipeResult( utils::readPipe( cmd ) );

    bool limitExceeded = false;
    std::string tempString;
    while( std::getline( pipeResult, tempString ) ) {
        if( jScanResults.size() == WIFI_SCAN_RESULTS_LIMIT ) {
            limitExceeded = true;
            break;
        }

        tempString.erase( std::remove( tempString.begin(), tempString.end(), ' ' ), tempString.end() );
        const std::string dbmToken = "dBm";
        tempString.replace( tempString.find( dbmToken ), dbmToken.length(), "" );
        std::istringstream iss( tempString );
        Json jNetworkData;
        while( std::getline( iss, tempString, ',' ) ) {
            auto delimitter = tempString.find( ':' );
            auto key = tempString.substr( 0, delimitter );
            auto value = tempString.substr( delimitter + 1, tempString.length() - delimitter + 1 );
            jNetworkData[ key ] = value;
        }
        jScanResults.push_back( std::move( jNetworkData ) );
    }

    testLog() << "Found " << (int) jScanResults.size() << ( ( limitExceeded ) ? "+" : "" ) << " wireless networks";

    return jScanResults;
}

void ConnectionTest::setUp( const ArgParser& argParser ) {
    try {
        const std::string argPingAddr = "--ping-addr";

        if( false == argParser.hasArg( argPingAddr ) )
            return;

        std::string addr = argParser.getValue( argPingAddr );
        if( !ArgUtils::isIpAddr( addr ) )
            throw ArgumentError( "Ping address has wrong format" );
        m_pingAddr = addr;
    }
    catch( ArgumentError& e ) {
        testLog() << "Error! " << e.what();
        argParser.help();
        exit( -1 );
    }
}

}  // namespace HardwareTest
}  // namespace HubPlatform
