#include "hardware_test/hardware_tests/lte_test.h"

#ifndef __LTE_TEST_SUPPORTED__
#error Macro __LTE_TEST_SUPPORTED__ should be defined in Makefile
#endif

#if __LTE_TEST_SUPPORTED__

#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <thread>
#include <chrono>
#include <regex>
#include <cassert>
#include <functional>
#include <set>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "base_utils/timeutils.h"
#include "base_utils/gpio.h"
#include "base_utils/uart/uart.h"
#include "base_utils/utils.h"
#include "jsonrpc/jsonrpc.h"

#include "hardware_test/utils/test_log.h"
#include "hardware_test/utils/lte_module.h"

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "LTE"
constexpr std::chrono::seconds TEST_TIMEOUT{ 180 };

namespace {
TestLog testLog( TEST_NAME );

class NoSIMCardError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

void testSimCard( LteModule& module, JsonObject& testSimStatus, JsonObject& testSimICCID, JsonObject& uniqueICCID ) {
    for( size_t attempt = 1; attempt < 3; ++attempt ) {
        testLog() << "Test SIM card: attempt: " << attempt;

        testSimStatus[ "simStatus" ] = "";
        testSimStatus[ "result" ] = false;

        testSimICCID[ "simICCID" ] = "";
        testSimICCID[ "result" ] = false;

        std::string iccid;

        try {
            auto status = module.getSimStatus();
            std::string statusStr = "LTE SIM status: " + module.simStatusStr( status );
            testLog() << statusStr;

            if( status == LteModule::SimStatus::NotInserted )
                throw NoSIMCardError( "SIM card is not inserted" );

            if( status == LteModule::SimStatus::Unknown )
                throw std::runtime_error( "SIM status: unknown" );

            assert( status == LteModule::SimStatus::Inserted );
            testSimStatus[ "simStatus" ] = module.simStatusStr( status );
            testSimStatus[ "result" ] = true;

            std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
            iccid = module.getSimICCID();
            testLog() << "SIM card ICCID: " << iccid;
        }
        catch( NoSIMCardError& e ) {
            throw;
        }
        catch( std::runtime_error& e ) {
            testLog() << "Error! " << e.what();
            module.reboot();
            module.waitForSerial();
            std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
            continue;
        }

        testSimICCID[ "simICCID" ] = iccid;
        uniqueICCID = JSON( "iccid_lte_0" << iccid );
        testSimICCID[ "result" ] = true;
        return;
    }
    throw std::runtime_error( "Attempts to test LTE SIM card have been exhausted" );
}

/* simple json rpc client over tcp socket */
class JsonRPCClient final {
public:
    JsonRPCClient( sockaddr_in serverAddress );
    ~JsonRPCClient();

    Json sendRequest( const Json& request );

private:
    sockaddr_in m_serverAddress;
    int m_socket = -1;
};

JsonRPCClient::JsonRPCClient( sockaddr_in serverAddress )
: m_serverAddress( serverAddress ) {
    m_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( m_socket < 0 ) {
        throw std::runtime_error( "JSONRPC: Failed to create a socket object" );
    }

    if( connect( m_socket, reinterpret_cast< struct sockaddr* >( &m_serverAddress ), sizeof( m_serverAddress ) ) < 0 ) {
        throw std::runtime_error( "JSONRPC: Failed to connect to a JSON RPC server" );
    }
}

JsonRPCClient::~JsonRPCClient() {
    close( m_socket );
}

Json JsonRPCClient::sendRequest( const Json& request ) {
    if( request.count( "jsonrpc" ) == 0 || request.count( "method" ) == 0 || request.count( "id" ) == 0 ) {
        throw std::runtime_error( "JSONRPC: Invalid request" );
    }

    const auto data = request.dump();
    auto ret = send( m_socket, data.c_str(), data.size(), 0 );
    if( ret != data.size() ) {
        throw std::runtime_error( "JSONRPC: Failed to send a request to a JSON RPC server" );
    }

    std::vector< char > buff( 4096 );
    ret = read( m_socket, buff.data(), buff.size() );
    if( ret < 0 ) {
        throw std::runtime_error( "JSONRPC: Failed to get a reply from a JSON RPC server" );
    }

    const auto reply = Json::parse( std::string( buff.begin(), buff.begin() + ret ) );
    if( reply.count( "jsonrpc" ) == 0 || reply.count( "id" ) == 0 ||
        ( reply.count( "result" ) == 0 && reply.count( "error" ) == 0 ) ||
        ( reply.count( "result" ) == 1 && reply.count( "error" ) == 1 ) ) {
        throw std::runtime_error( "JSONRPC: Invalid reply reply from a JSON RPC server" );
    }

    return reply;
}

class BandSync final {
    struct BandSyncObject {
        BandSyncObject( const Json& json );

        int m_bandNumber;
        int m_timeToBandStart;      // ms
        int m_timeToBandStop;       // ms
        int m_timeToNextBandStart;  // ms
    };

    friend std::ostream& operator<<( std::ostream& os, const BandSync::BandSyncObject& bandSyncObject );

public:
    using BandNumber = int;
    /* we have no remote control of an LTE callbox, we're just provided with a time window
     * we may make a measurement/test in, a measurment/test which goes beyond this time window is invalid,
     * BandSync::m_measurementTime represents this time window */
    using Test = std::function< void( BandSync::BandNumber bandNumber ) >;

    BandSync( const std::string& callboxAddr );

    void catchBandAndExecute( const Test& test );

private:
    BandSyncObject getBandSyncObject();

private:
    static constexpr std::chrono::milliseconds m_measurementTime = std::chrono::milliseconds( 12000 );
    sockaddr_in m_lteCallboxServiceAddress;
    const int m_lteCallboxServicePort = 8210;
};

constexpr std::chrono::milliseconds BandSync::m_measurementTime;

BandSync::BandSyncObject::BandSyncObject( const Json& json ) {
    if( json.count( "band" ) == 0 || json.count( "time_to_band_start" ) == 0 ||
        json.count( "time_to_band_stop" ) == 0 || json.count( "time_to_next_band_start" ) == 0 ) {
        throw std::runtime_error( "BandSyncObject: Failed to construct a BandSyncObject" );
    }

    m_bandNumber = json[ "band" ];
    m_timeToBandStart = json[ "time_to_band_start" ];
    m_timeToBandStop = json[ "time_to_band_stop" ];
    m_timeToNextBandStart = json[ "time_to_next_band_start" ];
}

std::ostream& operator<<( std::ostream& os, const BandSync::BandSyncObject& bandSyncObject ) {
    return os << "bandNumber: " << bandSyncObject.m_bandNumber
              << ", timeToBandStart: " << bandSyncObject.m_timeToBandStart
              << "ms, timeToBandStop: " << bandSyncObject.m_timeToBandStop
              << "ms, timeToNextBandStart: " << bandSyncObject.m_timeToNextBandStart << "ms\n";
}

BandSync::BandSync( const std::string& callboxIPAddress ) {
    bzero( &m_lteCallboxServiceAddress, sizeof( m_lteCallboxServiceAddress ) );

    m_lteCallboxServiceAddress.sin_family = AF_INET;
    m_lteCallboxServiceAddress.sin_port = htons( m_lteCallboxServicePort );
    if( inet_pton( AF_INET, callboxIPAddress.c_str(), &( m_lteCallboxServiceAddress.sin_addr.s_addr ) ) <= 0 ) {
        throw std::runtime_error( "BandSync: Invalid an LTE Callbox service IPv4 address" );
    }
}

void BandSync::catchBandAndExecute( const Test& test ) {
    const auto bandSyncObject = getBandSyncObject();
    auto bandNumber = bandSyncObject.m_bandNumber;

    if( bandSyncObject.m_timeToBandStart > 0 ) {
        testLog() << "band " << bandNumber
                  << " isn't started yet, wait " + std::to_string( bandSyncObject.m_timeToBandStart ) + "ms for it...";
        std::this_thread::sleep_for( std::chrono::milliseconds( bandSyncObject.m_timeToBandStart ) );
    }
    else {
        if( std::chrono::milliseconds( bandSyncObject.m_timeToBandStop ) < m_measurementTime ) {
            testLog() << "have no time to execute a test within this band, wait " +
                             std::to_string( bandSyncObject.m_timeToNextBandStart ) + "ms for the next one...";
            std::this_thread::sleep_for( std::chrono::milliseconds( bandSyncObject.m_timeToNextBandStart ) );
            bandNumber = getBandSyncObject().m_bandNumber;
        }
        else {
            testLog() << "band was just started, execute a test";
        }
    }

    test( bandNumber );
}

BandSync::BandSyncObject BandSync::getBandSyncObject() {
    JsonRPCClient jsonRPCClient( m_lteCallboxServiceAddress );
    const auto reply = jsonRPCClient.sendRequest( R"({ "method": "get_sync_obj", "id": "", "jsonrpc": "2.0" })"_json );
    if( hasError( reply ) ) {
        throw std::runtime_error( "BandSync: Error in a response on a get_sync_obj method: " + reply.dump( 1 ) );
    }

    return BandSyncObject( reply[ "result" ] );
}

using BandRSSIEdgeValues = std::map< BandSync::BandNumber, std::pair< int, int > >;

/* band_number:min_value:max_value, band_number:min_value:max_value, ..., band_number:min_value:max_value */
BandRSSIEdgeValues processBandRSSIEdgeValuesArg( const std::vector< std::string >& bandRSSIEdgeValuesArg ) {
    BandRSSIEdgeValues bandRSSIEdgeValues;
    std::string delimeter( ":" );

    for( const auto& elm : bandRSSIEdgeValuesArg ) {
        const auto firstDelimeterPos = elm.find( delimeter );
        if( firstDelimeterPos == std::string::npos ) {
            throw std::runtime_error( "Failed to parse bandRSSIEdgeValues argument: missing delimiter" );
        }
        const auto secondDelimeterPos = elm.find( delimeter, firstDelimeterPos + 1 );
        if( secondDelimeterPos == std::string::npos ) {
            throw std::runtime_error( "Failed to parse bandRSSIEdgeValues argument: missing delimiter" );
        }

        const auto bandString = elm.substr( 0, firstDelimeterPos );
        const auto minValueString = elm.substr( firstDelimeterPos + 1, secondDelimeterPos - firstDelimeterPos - 1 );
        const auto maxValueString = elm.substr( secondDelimeterPos + 1 );

        int band, minValue, maxValue;

        try {
            band = std::stoi( bandString );
            minValue = std::stoi( minValueString );
            maxValue = std::stoi( maxValueString );
            if( minValue < -113 || minValue > -51 || maxValue < -113 || maxValue > -51 || minValue > maxValue ||
                band < 1 || band > 88 ) {
                throw std::runtime_error( "RSSI exceeded supported values" );
            }
        }
        catch( const std::exception& ex ) {
            throw std::runtime_error( "Failed to parse bandRSSIEdgeValues argument: " + std::string( ex.what() ) );
        }

        bandRSSIEdgeValues.insert( std::make_pair( band, std::make_pair( minValue, maxValue ) ) );
    }

    return bandRSSIEdgeValues;
}

std::set< BandSync::BandNumber > chooseBandsToCheck( const BandRSSIEdgeValues& bandRSSIEdgeValues ) {
    std::set< BandSync::BandNumber > bandsToCheck;

    for( const auto& elm : bandRSSIEdgeValues ) {
        bandsToCheck.insert( elm.first );
    }

    return bandsToCheck;
}

void testRFPart( LteModule& lteModule,
                 JsonObject& testRF,
                 const std::string& callboxAddr,
                 const std::vector< std::string >& bandRSSIEdgeValuesArg,
                 const std::chrono::seconds& getRssiDelay ) {
    BandSync bandSync( callboxAddr );
    int currentBand;

    Json bandsRssi = JSON_EMPTY_ARR;
    bool passed = true;

    if( getRssiDelay > std::chrono::seconds( 60 ) ) {
        throw std::runtime_error( "Invalid delay to sleep before obtaining an rssi value" );
    }

    const auto bandRSSIEdgeValues = processBandRSSIEdgeValuesArg( bandRSSIEdgeValuesArg );
    auto bandsToCheck = chooseBandsToCheck( bandRSSIEdgeValues );

    auto rssiMatch = [&bandRSSIEdgeValues]( BandSync::BandNumber bandNumber, int rssi ) {
        auto it = bandRSSIEdgeValues.find( bandNumber );
        if( it != bandRSSIEdgeValues.end() ) {
            const auto minValue = it->second.first;
            const auto maxValue = it->second.second;

            return ( rssi <= maxValue ) && ( rssi >= minValue );
        }

        return false;
    };

    auto getRSSIMinMaxAsJson = [&bandRSSIEdgeValues]( BandSync::BandNumber bandNumber ) {
        auto it = bandRSSIEdgeValues.find( bandNumber );
        if( it != bandRSSIEdgeValues.end() ) {
            const auto minValue = it->second.first;
            const auto maxValue = it->second.second;

            return JSON( "min" << minValue << "max" << maxValue );
        }

        return JSON_EMPTY_OBJ;
    };

    auto calculateRssi = [&lteModule, &getRssiDelay]() {
        // we need to wait so much to guarantee we make a proper measurement
        std::this_thread::sleep_for( getRssiDelay );
        return lteModule.getRSSI();
    };

    auto testBand = [&]( BandSync::BandNumber bandNumber ) {
        currentBand = bandNumber;
        testLog() << "perform an LTE RF test for a band " << currentBand << "...";

        lteModule.reattach();
        const auto rssi = calculateRssi();
        lteModule.disattach();

        if( rssiMatch( currentBand, rssi ) ) {
            testLog() << "rrsi: " << rssi
                      << " dBm remains within valid boundaries " + getRSSIMinMaxAsJson( currentBand ).dump();
        }
        else {
            testLog() << "rrsi: " << rssi
                      << " dBm exceeded valid boundaries " + getRSSIMinMaxAsJson( currentBand ).dump();

            passed = false;
        }

        auto resultJson = getRSSIMinMaxAsJson( currentBand );

        resultJson[ "band" ] = std::to_string( currentBand );
        resultJson[ "value" ] = rssi;
        bandsRssi.push_back( resultJson );
    };

    while( true ) {
        bandSync.catchBandAndExecute( testBand );
        bandsToCheck.erase( currentBand );
        if( bandsToCheck.empty() ) {
            break;
        }
    }

    testRF[ "bandsRssi" ] = std::move( bandsRssi );
    testRF[ "result" ] = passed;

    if( !passed ) {
        throw std::runtime_error( json_get_str( testRF, "name" ) + " failed" );
    }
}

}  // namespace

LteTest::LteTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef )
, m_getRssiDelay( 5 ) {}

HardwareTestInterface::TestResult LteTest::run() {
    auto make_test = []( const char* name ) { return JSON( "name" << name << "result" << false ); };
    auto pass = []( JsonObject& j ) { j[ "result" ] = true; };
    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;

    auto testModuleInit = make_test( "module init" );
    auto testIMEI = make_test( "get imei" );
    auto testSimStatus = make_test( "get simStatus" );
    auto testSimICCID = make_test( "get simICCID" );
    auto testRF = make_test( "test RF" );
    auto testResults = make_test( m_name.c_str() );

    testLog() << "Starting " << m_name << " test...";
    auto startTestTime = TimeUtils::getMonotonicTime();
    try {
        std::chrono::seconds modemWarmUpTime{ 3 };
        LteModule lteModule( testLog );

        lteModule.wakeUp( modemWarmUpTime );
        pass( testModuleInit );

        auto imei = lteModule.getIMEI();
        testIMEI[ "imei" ] = imei;
        m_uniqueIMEI = JSON( "imei_lte_0" << imei );
        pass( testIMEI );

        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
        testSimCard( lteModule, testSimStatus, testSimICCID, m_uniqueICCID );

        if( m_hasCallbox ) {
            testRFPart( lteModule, testRF, m_callboxIPAddress, m_bandRSSIEdgeValuesArg, m_getRssiDelay );
        }

        result = TestResult::DONE;
    }
    catch( std::runtime_error& e ) {
        result = TestResult::FAILED;
        testLog() << "Error! " << e.what();
    }

    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    testResults[ "result" ] = ( result == TestResult::DONE ) ? true : false;
    JsonArray subtests = { testModuleInit, testIMEI, testSimStatus, testSimICCID };

    if( m_hasCallbox ) {
        subtests.push_back( testRF );
    }

    size_t passed = std::count_if( subtests.begin(), subtests.end(), []( JsonObject& j ) { return j[ "result" ]; } );
    testResults[ "value" ] = passed;
    testResults[ "threshold" ] = subtests.size();
    testResults[ "subtests" ] = std::move( subtests );

    testLog() << getTestResultJsonReport( testResults );

    std::string resultStr = ( result == TestResult::DONE ) ? "SUCCESS" : "FAIL";
    testLog() << "test finished with " << resultStr;

    return result;
}

JsonObject LteTest::getUniqueIMEI() {
    return m_uniqueIMEI;
}

JsonObject LteTest::getUniqueICCID() {
    return m_uniqueICCID;
}

bool LteTest::hasUniqueICCID() {
    return m_hasUniqueICCID;
}

void LteTest::setUp( const ArgParser& argParser ) {
    try {
        const std::string argUniqueICCID = "--unique-iccid";
        const std::string argCallbox = "--callbox";
        const std::string argBandRSSIEdgeValues = "--lte-band-rssi-edge-values";
        const std::string argGetRssiDelay = "--lte-get-rssi-delay";

        if( argParser.hasArg( argUniqueICCID ) ) {
            std::string argValue = argParser.getValue( argUniqueICCID );
            if( argValue == "true" )
                m_hasUniqueICCID = true;
            else if( argValue == "false" )
                m_hasUniqueICCID = false;
            else
                throw ArgumentError( argUniqueICCID + " has invalid value: " + argValue );
        }

        if( argParser.hasArg( argCallbox ) ) {
            m_callboxIPAddress = argParser.getValue( argCallbox );
            if( !ArgUtils::isIpAddr( m_callboxIPAddress ) ) {
                throw ArgumentError( "LTE Callbox address has wrong format" );
            }
        }

        if( argParser.hasArg( argBandRSSIEdgeValues ) ) {
            m_bandRSSIEdgeValuesArg = argParser.getArray( argBandRSSIEdgeValues );
            if( m_bandRSSIEdgeValuesArg.empty() ) {
                throw std::runtime_error( "No rssi edge values (min, max) for bands" );
            }
        }

        if( argParser.hasArg( argGetRssiDelay ) ) {
            auto getRssiDelay = argParser.getValue( argGetRssiDelay );
            if( getRssiDelay.empty() ) {
                throw std::runtime_error( "No delay value to sleep before get rssi" );
            }

            m_getRssiDelay = std::chrono::seconds( std::stoi( getRssiDelay ) );
        }

        if( argParser.hasArg( argCallbox ) && argParser.hasArg( argBandRSSIEdgeValues ) ) {
            m_hasCallbox = true;
        }
    }
    catch( ArgumentError& e ) {
        std::cout << "Error! " << e.what() << std::endl << std::flush;
        argParser.help();
        exit( -1 );
    }
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#else  // __LTE_TEST_SUPPORTED__

namespace HubPlatform {
namespace HardwareTest {

HubPlatform::HardwareTest::LteTest::LteTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( "DummyLTE", std::chrono::seconds( 1 ), exitSignalRef ) {}
HardwareTestInterface::TestResult LteTest::run() {
    return HardwareTestInterface::TestResult::DONE;
}

void LteTest::setUp( const ArgParser& argParser ) {}

}  // namespace HardwareTest
}  // namespace HubPlatform

#endif  // __LTE_TEST_SUPPORTED__
