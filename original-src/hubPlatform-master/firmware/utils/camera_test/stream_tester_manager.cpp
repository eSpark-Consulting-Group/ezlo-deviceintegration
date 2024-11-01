#include "camera_test/stream_tester_manager.h"

#include <ostream>
#include <iomanip>

StreamTesterManager::StreamTesterManager( std::string urlForTesting,
                                          std::chrono::seconds connectionDuration,
                                          std::chrono::seconds delayDuration,
                                          std::chrono::seconds totalTestDuration,
                                          std::chrono::milliseconds packetDurationThreshold,
                                          std::ostream& outputStream )
: m_urlForTesting( std::move( urlForTesting ) )
, m_connectionDuration( connectionDuration )
, m_delayDuration( delayDuration )
, m_totalTestDuration( totalTestDuration )
, m_packetDurationThreshold( packetDurationThreshold )
, m_outputStream( outputStream ) {}

void StreamTesterManager::runKeyFrameTest() {
    printTestsHeader( m_outputStream );
    runTests();
}

void StreamTesterManager::runContinuousStreamingTest() {
    runTests();
}

void StreamTesterManager::runTests() {
    using Clock = std::chrono::steady_clock;

    auto packetDurationCallback = std::bind(
        &StreamTesterManager::printPacketDurationWarning, std::ref( m_outputStream ), std::placeholders::_1 );

    int counter = 0;

    const Clock::time_point testEndTimePoint = Clock::now() + m_totalTestDuration;
    while( Clock::now() < testEndTimePoint ) {
        StreamTester tester( m_urlForTesting, m_connectionDuration, m_packetDurationThreshold, packetDurationCallback );

        const StreamTester::TestResult result = tester.runTest();
        printTestResult( m_outputStream, ++counter, result );

        std::this_thread::sleep_for( m_delayDuration );
    }
}

void StreamTesterManager::printTestsHeader( std::ostream& outputStream ) {
    auto writeToStream = [&outputStream]( const std::string& string, int width ) {
        outputStream << std::setw( width ) << string;
    };

    writeToStream( "Test num", PrintValueWidth );
    writeToStream( "Packets", PrintValueWidth );
    writeToStream( "First packet", PrintValueWidth );
    writeToStream( "Key packet", PrintValueWidth );
    writeToStream( "Handshake", PrintValueWidth );
    outputStream << '\n';

    writeToStream( "count", PrintValueWidth * 2 );
    writeToStream( "is key", PrintValueWidth );
    writeToStream( "time, ms", PrintValueWidth );
    writeToStream( "time, ms", PrintValueWidth );

    outputStream << std::endl;
}

void StreamTesterManager::printTestResult( std::ostream& outputStream,
                                           int testNumber,
                                           const StreamTester::TestResult& testResult ) {
    outputStream << std::setw( PrintValueWidth ) << std::to_string( testNumber );

    if( testResult.hasError ) {
        outputStream << " error: " << testResult.errorMessage;
    }
    else {
        const StreamStatistic& statistic = testResult.statistic;
        outputStream << std::setw( PrintValueWidth ) << std::to_string( statistic.packetsCount );
        outputStream << std::setw( PrintValueWidth ) << statistic.firstVideoPacketIsKey;
        outputStream << std::setw( PrintValueWidth ) << std::to_string( statistic.keyPacketTime.count() );
        outputStream << std::setw( PrintValueWidth ) << std::to_string( statistic.handshakeTime.count() );
    }

    outputStream << std::endl;
}

void StreamTesterManager::printPacketDurationWarning( std::ostream& outputStream,
                                                      std::chrono::milliseconds packetDuration ) {
    outputStream << "warning: packet duration is greater than the threshold, duration: "
                 << std::to_string( packetDuration.count() ) << " ms";
    outputStream << std::endl;
}
