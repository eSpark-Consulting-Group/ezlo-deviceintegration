#include "hardware_test/hardware_tests/recovery_button_test.h"

#include "hardware_test/utils/test_log.h"

#include "base_utils/timeutils.h"
#include "json_utils/json_utils.h"

#include <iostream>

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "Button"
constexpr std::chrono::seconds TEST_TIMEOUT{ 80 };

namespace {
TestLog testLog( TEST_NAME );
}  // namespace

RecoveryButtonTest::RecoveryButtonTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef )
, m_config() {}

HardwareTestInterface::TestResult RecoveryButtonTest::run() {
    bool findEzloButtonTestDone = false;
    bool powerOffButtonTestDone = false;

    static const std::string promtPressMessage = "Perform double press and long press (5 seconds ) within " +
                                                 std::to_string( (int) buttonTestTimeoutSeconds ) + " seconds...";
    const std::string doublePressTestName = "Double press test";
    const std::string longPressTestName = "Long press test";
    // number of tested events below
    const int testsPassThreshold = 2;
    int testsPassedValue = 0;

    testLog() << "Starting " << m_name << " test...";
    testLog() << promtPressMessage;

    auto startTestTime = TimeUtils::getMonotonicTime();
    JsonArray jSubTestsResults;
    while( !m_exitSignal && ( !findEzloButtonTestDone || !powerOffButtonTestDone ) &&
           TimeUtils::getDiffMonotonicTime( startTestTime ) < buttonTestTimeoutSeconds ) {
    }
    TestResult result = ( findEzloButtonTestDone && powerOffButtonTestDone ) ? TestResult::DONE : TestResult::FAILED;
    testLog() << getTestResultReport( result, TimeUtils::getDiffMonotonicTime( startTestTime ) );

    Json jTestResult;
    jTestResult[ "name" ] = m_name;
    jTestResult[ "result" ] = ( testsPassedValue == testsPassThreshold );
    jTestResult[ "threshold" ] = testsPassThreshold;
    jTestResult[ "value" ] = testsPassedValue;
    jTestResult[ "subtests" ] = jSubTestsResults;
    testLog() << getTestResultJsonReport( jTestResult );

    return result;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
