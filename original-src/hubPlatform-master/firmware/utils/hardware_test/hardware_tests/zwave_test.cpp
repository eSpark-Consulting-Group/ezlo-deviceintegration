#include "hardware_test/hardware_tests/zwave_test.h"

#include "hardware_test/utils/test_log.h"

#include "base_utils/timeutils.h"
#include "base_utils/utils.h"
#include "json_utils/json_utils.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <arpa/inet.h>

#define __ZWAVE_CHIP_PREFLASHED__
#ifndef __ZWAVE_CHIP_PREFLASHED__
#define __TEST_ERASE_CHIP__
#define __TEST_RESTORE_CHIP__
#endif

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "ZWave"
constexpr std::chrono::seconds TEST_TIMEOUT{ 80 };

namespace {
TestLog testLog( TEST_NAME );

#ifdef __ZWAVE_CHIP_PREFLASHED__
static const std::string ZwaveFlasherCmd = "/opt/firmware/bin/fb-zwflash -t";
#else
static const std::string ZwaveFlasherCmd = "/opt/firmware/bin/fb-zwflash -f";
#endif

static const std::string DefaultZwaveUartPortName = "/dev/ttyS1";
static const std::string subTestChipReadName = "chip read";
static const std::string subTestChipEraseName = "chip erase";
static const std::string subTestChipEraseMatchName = "chip erase/match";
static const std::string subTestChipRestoreName = "chip restore";
static const std::string subTestZwSerialApiStartedName = "serial api started";
static const std::string subTestZwGetDskName = "get DSK";
static const std::string subTestZwGetHomeIdName = "get homeId";
static const std::string subTestZwGetSendNifName = "send nif";

static constexpr uint8_t SerialApiSOF = 0x01;
static constexpr uint8_t SerialApiACK = 0x06;
static constexpr uint8_t SerialApiNACK = 0x15;
static constexpr uint8_t SerialApiRequest = 0x00;
static constexpr uint8_t SerialApiResponse = 0x01;
static constexpr uint8_t SerialApiFcStartedCode = 0x0A;
static constexpr uint8_t SerialApiFcGetHomeId = 0x20;
static constexpr uint8_t SerialApiFcGetNvr = 0x28;
static constexpr uint8_t SerialApiFcSendNif = 0x12;
static constexpr uint8_t SerialApiFcSetDefault = 0x42;

static const int testsPassMinimalThreshold = 4;
static const int restoreChipTestsCount = 6;
static const int eraseChipTestsCount = 2;
static int testsPassedValue = 0;

#if defined( __TEST_RESTORE_CHIP__ ) && defined( __TEST_ERASE_CHIP__ )
const int testsPassThreshold = testsPassMinimalThreshold + restoreChipTestsCount + eraseChipTestsCount;
#elif defined( __TEST_RESTORE_CHIP__ )
const int testsPassThreshold = testsPassMinimalThreshold + restoreChipTestsCount;
#elif defined( __TEST_ERASE_CHIP__ )
const int testsPassThreshold = testsPassMinimalThreshold + eraseChipTestsCount;
#else
const int testsPassThreshold = testsPassMinimalThreshold;
#endif  // defined( __TEST_RESTORE_CHIP__ ) || defined(__TEST_ERASE_CHIP__)

void jSubTestResultsAddtoArray( JsonArray& jSubResults,
                                const std::string& subTestName,
                                const bool subTestResult,
                                const Json& jTestObject = JSON_EMPTY_OBJ ) {
    jSubResults.push_back( JSON( "name" << subTestName << "result" << subTestResult << jTestObject ) );
}

}  // namespace

ZwaveTest::ZwaveTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef ) {}
/*
, m_serialApi( DefaultZwaveUartPortName, true )
, m_zwaveStick() {}
*/

HardwareTestInterface::TestResult ZwaveTest::run() {
    TestResult testResult = TestResult::DONE;
    TestResult subTestResult = TestResult::UNKNOWN;

    testLog() << "Starting " << m_name << " test ...";

    JsonArray jSubTestsResults;
    std::string subTestName;

    time_t startTestTime = TimeUtils::getMonotonicTime();
    time_t startSubTestTime = TimeUtils::getMonotonicTime();

#ifdef __TEST_RESTORE_CHIP__
    std::unique_ptr< ZWaveStick > zwaveStick = std::make_unique< ZWaveStick >();

    startSubTestTime = TimeUtils::getMonotonicTime();
    std::vector< uint8_t > zwaveFlashDataDump;
    std::vector< uint8_t > zwaveNvrDataDump;
    testLog() << "Creating Z-Wave chip dump...";
    subTestResult =
        ( zwaveStick->zwChipRead( zwaveNvrDataDump, zwaveFlashDataDump ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipReadName;
    jSubTestResultsAddtoArray( jSubTestsResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startSubTestTime ) );
#endif

#ifdef __TEST_ERASE_CHIP__
    static constexpr uint8_t zwChipEmptyValue = 0xFF;
    startSubTestTime = TimeUtils::getMonotonicTime();
    testLog() << "Erasing Z-Wave chip...";
    zwaveStick->zwChipErase();
    std::vector< uint8_t > zwaveFlashData;
    std::vector< uint8_t > zwaveNvrData;
    subTestResult = ( zwaveStick->zwChipRead( zwaveNvrData, zwaveFlashData ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipEraseName;
    jSubTestResultsAddtoArray( jSubTestsResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startSubTestTime ) );

    startSubTestTime = TimeUtils::getMonotonicTime();
    subTestResult = TestResult::DONE;
    for( std::vector< uint8_t >::const_iterator it = zwaveFlashData.begin(); it != zwaveFlashData.end(); ++it ) {
        if( *it != zwChipEmptyValue ) {
            subTestResult = TestResult::FAILED;
            testLog() << "Erase operation failed at: zwaveFlashData[ " << (int) ( it - zwaveFlashData.begin() )
                      << " ] = " << hexToString( it.base(), sizeof( uint8_t ) );
        }
    }
    for( std::vector< uint8_t >::const_iterator it = zwaveNvrData.begin(); it != zwaveNvrData.end(); ++it ) {
        if( *it != zwChipEmptyValue ) {
            subTestResult = TestResult::FAILED;
            testLog() << "Erase operation failed at: zwaveNvrData[ " << (int) ( it - zwaveNvrData.begin() )
                      << " ] = " << hexToString( it.base(), sizeof( uint8_t ) );
        }
    }
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipEraseMatchName;
    jSubTestResultsAddtoArray( jSubTestsResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startSubTestTime ) );
#endif

    subTestResult = testEnvm( jSubTestsResults );
    if( subTestResult == TestResult::FAILED ) {
        testResult = TestResult::FAILED;
    }

#ifdef __TEST_RESTORE_CHIP__
    zwaveStick = std::make_unique< ZWaveStick >();
    startSubTestTime = TimeUtils::getMonotonicTime();
    testLog() << "Restore Z-Wave chip...";
    subTestResult =
        ( zwaveStick->zwChipWrite( zwaveNvrDataDump, zwaveFlashDataDump ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = std::string( m_name ).append( " ( " ).append( subTestChipRestoreName ).append( " )" );
    jSubTestResultsAddtoArray( jSubTestsResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startSubTestTime ) );

    // free UART resource if acquired by zwaveStick
    zwaveStick.reset();
#endif

    subTestResult = testSerialApi( jSubTestsResults );
    if( subTestResult == TestResult::FAILED ) {
        testResult = TestResult::FAILED;
    }

    testLog() << getTestResultReport( testResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    Json jTestResult;
    jTestResult[ "name" ] = m_name;
    jTestResult[ "result" ] = testResult == TestResult::DONE ? true : false;
    jTestResult[ "threshold" ] = testsPassThreshold;
    jTestResult[ "value" ] = testsPassedValue;
    jTestResult[ "subtests" ] = jSubTestsResults;
    testLog() << getTestResultJsonReport( jTestResult );

    return testResult;
}

HardwareTestInterface::TestResult ZwaveTest::testEnvm( Json& jTestResults ) {
    static constexpr uint8_t envmEmptyValue = 0xFF;
    static constexpr char const envmSubTestName[] = "Z-Wave ( ENVM chip )";

    testLog() << "Starting " << envmSubTestName << "...";

    auto startTestTime = TimeUtils::getMonotonicTime();
    TestResult testResult = TestResult::DONE;
    TestResult subTestResult = TestResult::UNKNOWN;
    std::string subTestName;

#ifdef __TEST_RESTORE_CHIP__
    std::unique_ptr< ZWaveStick > zwaveStick = std::make_unique< ZWaveStick >();

    testLog() << "Creating ENVM chip dump...";
    std::vector< uint8_t > envmDataDump;
    subTestResult = ( zwaveStick->envmRead( envmDataDump ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipReadName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );
#endif

#ifdef __TEST_RESTORE_CHIP__
    startTestTime = TimeUtils::getMonotonicTime();
    testLog() << "Erasing ENVM chip...";
    subTestResult = zwaveStick->envmErase() ? TestResult::DONE : TestResult::FAILED;
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipEraseName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    startTestTime = TimeUtils::getMonotonicTime();
    std::vector< uint8_t > envmData;
    zwaveStick->envmRead( envmData );

    subTestResult = TestResult::DONE;
    for( std::vector< uint8_t >::const_iterator it = envmData.begin(); it != envmData.end(); ++it ) {
        if( *it != envmEmptyValue ) {
            subTestResult = TestResult::FAILED;
            testLog() << "Erase operation failed at: envmData[ " << (int) ( it - envmData.begin() )
                      << " ] = " << hexToString( it.base(), sizeof( uint8_t ) );
        }
    }
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipEraseMatchName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );
#endif

#ifdef __TEST_RESTORE_CHIP__
    startTestTime = TimeUtils::getMonotonicTime();
    testLog() << "Restore ENVM chip...";
    subTestResult = ( zwaveStick->envmWrite( envmDataDump ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestChipRestoreName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    zwaveStick.reset();
#endif

    testLog() << getTestResultReport( envmSubTestName, testResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    return testResult;
}

HardwareTestInterface::TestResult ZwaveTest::testSerialApi( Json& jTestResults ) {
    static constexpr char const subTestSerialApiName[] = "Z-Wave ( Serial API )";

    testLog() << "Starting " << subTestSerialApiName << "...";

    TestResult testResult = TestResult::DONE;
    TestResult subTestResult = TestResult::UNKNOWN;
    std::string subTestName;

    auto startTestTime = TimeUtils::getMonotonicTime();

    // wait until uart has valid state
    for( size_t i = 0; i < 3; ++i ) {
        ZwaveSerialApi serialApi( DefaultZwaveUartPortName, true, m_exitSignal );
        if( serialApi.waitSerialApiStarted( 3 ) )
            break;
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    }

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

#if !defined( __ZWAVE_CHIP_PREFLASHED__ ) || ( !defined( __HUB_TYPE_H2__ ) && !defined( __HUB_TYPE_H2_SECURE__ ) )
    system( ZwaveFlasherCmd.c_str() );
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
#endif

    std::unique_ptr< ZwaveSerialApi > serialApi;
    bool isApiStarted = false;
    for( size_t i = 0; i < 3; ++i ) {
        serialApi = std::make_unique< ZwaveSerialApi >( DefaultZwaveUartPortName, true, m_exitSignal );
        isApiStarted = serialApi->waitSerialApiStarted( 3 );
        if( isApiStarted )
            break;
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    }

    subTestResult = ( isApiStarted ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestZwSerialApiStartedName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult );
    testLog() << getTestResultReport( subTestName, subTestResult );

    serialApi->sendSetDefault();

    std::string homeId;
    subTestResult = ( serialApi->getHomeId( homeId ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestZwGetHomeIdName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult, JSON( "homeId" << homeId ) );
    testLog() << getTestResultReport( subTestName, subTestResult );
    if( !homeId.empty() ) {
        testLog() << "homeId: " << homeId;
    }

    std::string dsk;
    subTestResult = ( serialApi->getDsk( dsk ) ? TestResult::DONE : TestResult::FAILED );
    if( subTestResult != TestResult::DONE ) {
        testResult = TestResult::FAILED;
    }
    else {
        ++testsPassedValue;
    }
    subTestName = subTestZwGetDskName;
    jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult, JSON( "dsk" << dsk ) );
    m_uniqueDSK = JSON( "dsk_zwave_0" << dsk );
    testLog() << getTestResultReport( subTestName, subTestResult );
    if( !dsk.empty() ) {
        testLog() << "dsk: " << dsk;
    }
    if( !homeId.empty() ) {
        subTestResult = ( serialApi->sendNif() ? TestResult::DONE : TestResult::FAILED );
        if( subTestResult != TestResult::DONE ) {
            testResult = TestResult::FAILED;
        }
        else {
            ++testsPassedValue;
        }
        subTestName = subTestZwGetSendNifName;
        jSubTestResultsAddtoArray( jTestResults, subTestName, subTestResult );
        testLog() << getTestResultReport( subTestName, subTestResult );
    }

    testLog() << getTestResultReport(
        subTestSerialApiName, testResult, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    return testResult;
}

JsonObject ZwaveTest::getUniqueDSK() {
    return m_uniqueDSK;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
