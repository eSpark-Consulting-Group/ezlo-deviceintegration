#include "hardware_test/hardware_tester.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include "system_utils/environment.h"
#include "json_utils/json_utils.h"

#include "hardware_test/arg_parser.h"
#include "hardware_test/task_visitor.h"

namespace HubPlatform {
namespace HardwareTest {

namespace {

static const std::string StopScriptPath = Environment::getBinariesFolder().append( "/stop.sh" );
static const std::string GetSerialCommand = Environment::getBinariesFolder().append( "/at88util -p -s" );
static const std::string GetMacCommand = Environment::getBinariesFolder().append( "/at88util -p -m" );
static const std::string GetUniqueJsonCommand = Environment::getBinariesFolder().append( "/at88util --unique-json" );
static const std::string MigrationToolPath = Environment::getBinariesFolder().append( "/migration_tool" );

JsonObject readUniqueJson() {
    std::array< char, 4096 > buffer;
    std::string result;
    std::unique_ptr< FILE, decltype( &pclose ) > pipe( popen( GetUniqueJsonCommand.c_str(), "r" ), pclose );
    if( !pipe ) {
        throw std::runtime_error( "Can't open pipe!" );
    }
    while( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr ) {
        result += buffer.data();
    }
    JsonObject json_result;
    if( !parseJson( result, json_result ) )
        throw std::runtime_error( "Can't parse unique json!" );
    return json_result;
}

void extendUniqueHardwareJson( JsonObject& base, const JsonObject& ext ) {
    if( ext.empty() )
        throw std::runtime_error( "Unique extension is empty!" );
    for( auto it = ext.cbegin(); it != ext.cend(); ++it )
        base[ "unique" ][ "hardware" ][ it.key() ] = it.value();
}

void reportUniqueJson( JsonObject& json ) {
    std::cout << "unique-json: " << json.dump() << std::endl << std::flush;
}

}  // namespace

HardwareTester::HardwareTester( const volatile sig_atomic_t& exitSignalRef )
: m_exitSignal( exitSignalRef )
, m_emotionLedTest( exitSignalRef )
, m_soundTest( exitSignalRef )
, m_batteryTest( exitSignalRef )
, m_usbTest( exitSignalRef )
, m_recoveryButtonTest( exitSignalRef )
, m_zwaveTest( exitSignalRef )
, m_zigbeeTest( exitSignalRef )
, m_bluetoothTest( exitSignalRef )
, m_lteTest( exitSignalRef )
, m_connectionTest( exitSignalRef ) {
    m_allTests = { &m_emotionLedTest, &m_soundTest,     &m_batteryTest, &m_usbTest,       &m_zigbeeTest,
                   &m_zwaveTest,      &m_bluetoothTest, &m_lteTest,     &m_connectionTest };
}

void HardwareTester::setUp( const std::vector< std::string >& args ) {
    std::vector< std::string > allTestNames;
    for( auto test : m_allTests )
        allTestNames.emplace_back( test->getName() );

    ArgParser argParser( args, allTestNames );
    m_emotionLedTest.setUp( argParser );
    m_soundTest.setUp( argParser );
    m_connectionTest.setUp( argParser );
    m_lteTest.setUp( argParser );
    setUpTester( argParser );

    if( access( StopScriptPath.c_str(), X_OK ) != -1 ) {
        system( StopScriptPath.c_str() );
    }
    else {
        throw std::runtime_error( std::string( __PRETTY_FUNCTION__ ).append( ": stop script NOT found !" ) );
    }
    if( access( MigrationToolPath.c_str(), X_OK ) != -1 ) {
        system( MigrationToolPath.c_str() );
    }
    else {
        throw std::runtime_error( std::string( __PRETTY_FUNCTION__ ).append( ": migration binary NOT found !" ) );
    }
}

int HardwareTester::runTestLoop() {
    static constexpr char const startLoopMessage[] = "\n Start running test loop...\n";
    static constexpr char const endLoopMessage[] = "\n Test loop finished\n";

    std::cout << startLoopMessage << std::endl;
    try {
        auto taskQueue = createTaskQueue( m_tests, m_execOrder );
        taskQueue->start();

        taskQueue->accept( JoinVisitor() );

        StatusVisitor statusVisitor;
        taskQueue->accept( statusVisitor );
        const bool totalResult = statusVisitor.totalResult;

        printUniqueData();
        auto uniqueJson = readUniqueJson();

        auto testExecuted = [this]( const HardwareTestBase* test ) {
            return std::find( m_tests.cbegin(), m_tests.cend(), test ) != m_tests.cend();
        };

        if( testExecuted( &m_zwaveTest ) )
            extendUniqueHardwareJson( uniqueJson, m_zwaveTest.getUniqueDSK() );

#if __ZIGBEE_TEST_SUPPORTED__
        if( testExecuted( &m_zigbeeTest ) )
            extendUniqueHardwareJson( uniqueJson, m_zigbeeTest.getUniqueEUI64() );
#endif

#if __LTE_TEST_SUPPORTED__
        if( testExecuted( &m_lteTest ) ) {
            extendUniqueHardwareJson( uniqueJson, m_lteTest.getUniqueIMEI() );
            if( m_lteTest.hasUniqueICCID() )
                extendUniqueHardwareJson( uniqueJson, m_lteTest.getUniqueICCID() );
        }
#endif

        reportUniqueJson( uniqueJson );
        std::cout << endLoopMessage << std::endl;

        std::cout << "TEST RESULTS: \n" + statusVisitor.report.str() << std::endl;

        std::cout << "HARDWARE_TEST_TOTAL_RESULT: " << ( ( totalResult ) ? "SUCCESS" : "FAILURE" ) << std::endl;
        std::cout << std::flush;
        return ( totalResult ) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch( std::exception& e ) {
        std::cout << "Exception occurs while running test loop" << std::endl;
        std::cout << e.what() << std::endl << std::flush;
        return EXIT_FAILURE;
    }
}

void HardwareTester::printUniqueData() {
    system( GetSerialCommand.c_str() );
    system( GetMacCommand.c_str() );
}

void HardwareTester::setUpTester( const ArgParser& argParser ) {
    try {
        const std::string argExecOrder = "--exec-order";

        if( argParser.hasArg( argExecOrder ) ) {
            std::string argValue = argParser.getValue( argExecOrder );
            if( argValue == "parallel" )
                m_execOrder = TaskQueue::Order::Parallel;
            else if( argValue == "serial" )
                m_execOrder = TaskQueue::Order::Serial;
            else
                throw ArgumentError( argExecOrder + " has invalid value: " + argValue );
        }

        setupSupportedTests( argParser );
    }
    catch( ArgumentError& e ) {
        std::cout << "Error! " << e.what() << std::endl << std::flush;
        argParser.help();
        exit( -1 );
    }
}

void HardwareTester::setupSupportedTests( const ArgParser& argParser ) {
    std::map< std::string, HardwareTestBase* > allTestsMap;
    for( auto* test : m_allTests )
        allTestsMap.emplace( test->getName(), test );

    auto getTestSet = [&allTestsMap]( const std::vector< std::string >& tests ) {
        std::set< HardwareTestBase* > testSet;

        for( auto& testName : tests ) {
            auto it = allTestsMap.find( testName );
            if( it == allTestsMap.cend() )
                throw ArgumentError( "Test name \"" + testName + "\" not recognized" );
            testSet.emplace( it->second );
        }
        return testSet;
    };

    const std::string argExclude = "--exclude";
    const std::string argSelect = "--select";

    if( argParser.hasArg( argExclude ) && argParser.hasArg( argSelect ) ) {
        std::cout << "Wrong usage of --select and --exclude flags" << std::endl;
        argParser.help();
        exit( -1 );
    }

    try {
        enum class Mode { Full, Select, Exclude };
        Mode mode = Mode::Full;

        if( argParser.hasArg( argExclude ) )
            mode = Mode::Exclude;

        if( argParser.hasArg( argSelect ) )
            mode = Mode::Select;

        std::set< HardwareTestBase* > testSet;

        if( mode == Mode::Full ) {
            m_tests = m_allTests;
        }
        else if( mode == Mode::Select ) {
            auto testSet = getTestSet( argParser.getArray( argSelect ) );
            auto testFound = [&testSet]( HardwareTestBase* ptr ) { return testSet.find( ptr ) != testSet.end(); };
            std::copy_if( m_allTests.begin(), m_allTests.end(), std::back_inserter( m_tests ), testFound );
        }
        else if( mode == Mode::Exclude ) {
            auto testSet = getTestSet( argParser.getArray( argExclude ) );
            auto testNotFound = [&testSet]( HardwareTestBase* ptr ) { return testSet.find( ptr ) == testSet.end(); };
            std::copy_if( m_allTests.begin(), m_allTests.end(), std::back_inserter( m_tests ), testNotFound );
        }

        std::cout << "Selected tests: ";
        for( auto& test : m_tests )
            std::cout << test->getName() << ", ";
        std::cout << std::endl;
    }
    catch( ArgumentError& e ) {
        std::cout << "Error! " << e.what() << std::endl << std::flush;
        argParser.help();
        exit( -1 );
    }
}

std::unique_ptr< TaskQueue > HardwareTester::createTaskQueue( const std::vector< HardwareTestBase* >& tests,
                                                              TaskQueue::Order order ) {
    auto appendIfSelected = [tests]( TaskQueue* queue, HardwareTestBase& test ) {
        if( std::find( tests.cbegin(), tests.cend(), &test ) != tests.cend() )
            queue->add( std::make_unique< TaskTest >( test ) );
    };

    auto appendIfNotEmpty = []( TaskQueue* parentQueue, std::unique_ptr< TaskQueue >&& childQueue ) {
        if( childQueue->size() )
            parentQueue->add( std::move( childQueue ) );
    };

    auto manualTests = std::make_unique< TaskQueue >( TaskQueue::Order::Serial );
    appendIfSelected( manualTests.get(), m_emotionLedTest );
    appendIfSelected( manualTests.get(), m_soundTest );

    auto modemRelatedTests = std::make_unique< TaskQueue >( TaskQueue::Order::Serial );
    appendIfSelected( modemRelatedTests.get(), m_batteryTest );
    appendIfSelected( modemRelatedTests.get(), m_lteTest );

    auto generalQueue = std::make_unique< TaskQueue >( order );
    appendIfNotEmpty( generalQueue.get(), std::move( manualTests ) );
    appendIfNotEmpty( generalQueue.get(), std::move( modemRelatedTests ) );
    appendIfSelected( generalQueue.get(), m_usbTest );
    appendIfSelected( generalQueue.get(), m_zigbeeTest );
    appendIfSelected( generalQueue.get(), m_zwaveTest );
    appendIfSelected( generalQueue.get(), m_bluetoothTest );

    auto rootQueue = std::make_unique< TaskQueue >( TaskQueue::Order::Serial );
    appendIfNotEmpty( rootQueue.get(), std::move( generalQueue ) );
    appendIfSelected( rootQueue.get(), m_connectionTest );
    return rootQueue;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
