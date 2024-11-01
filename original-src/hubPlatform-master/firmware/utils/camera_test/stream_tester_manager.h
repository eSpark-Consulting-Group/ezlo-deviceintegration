#pragma once

#include "camera_test/stream_tester.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <thread>
#include <map>

class StreamTesterManager {
public:
    StreamTesterManager( std::string urlForTesting,
                         std::chrono::seconds connectionDuration,
                         std::chrono::seconds delayDuration,
                         std::chrono::seconds totalTestDuration,
                         std::chrono::milliseconds packetDurationThreshold,
                         std::ostream& outputStream );

    void runKeyFrameTest();
    void runContinuousStreamingTest();

private:
    void runTests();

    static void printTestsHeader( std::ostream& outputStream );
    static void printTestResult( std::ostream& outputStream,
                                 int testNumber,
                                 const StreamTester::TestResult& testResult );
    static void printPacketDurationWarning( std::ostream& outputStream, std::chrono::milliseconds packetDuration );

private:
    std::string m_urlForTesting;
    std::ostream& m_outputStream;

    std::chrono::seconds m_connectionDuration{ 0 };
    std::chrono::seconds m_delayDuration{ 0 };
    std::chrono::seconds m_totalTestDuration{ 0 };
    std::chrono::milliseconds m_packetDurationThreshold{ 0 };

    static constexpr int PrintValueWidth{ 14 };
};
