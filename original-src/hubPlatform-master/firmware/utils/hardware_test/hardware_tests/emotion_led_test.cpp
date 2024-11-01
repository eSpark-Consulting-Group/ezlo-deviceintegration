#include "hardware_test/hardware_tests/emotion_led_test.h"

#include "hardware_test/utils/test_log.h"

#include "base_utils/timeutils.h"
#include "json_utils/json_utils.h"

#include <iostream>

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "LED"
constexpr std::chrono::seconds TEST_TIMEOUT{ 80 };

namespace {
TestLog testLog( TEST_NAME );
static constexpr std::chrono::milliseconds testLoopDelayMs( 40 );
}  // namespace

EmotionLedTest::EmotionLedTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef )
, m_config()
, m_emotionLed( m_config.getEmotionLed() )
, m_button( m_config.getResetButtonNumber() ) {
    m_previousButtonState = m_button.getState();
    m_currentButtonState = m_button.getState();
}

HardwareTestInterface::TestResult EmotionLedTest::run() {
    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;

    std::vector< std::pair< std::string, Rgb > > testEffects = {
        { "RED", Rgb::Red },
        { "GREEN", Rgb::Green },
        { "BLUE", Rgb::Blue },
    };

    testLog() << "Starting " << m_name << " test...";
    auto startTestTime = TimeUtils::getMonotonicTime();

    std::string currentEffect;
    JsonArray jSubTestsResults;
    const int testsPassThreshold = testEffects.size();
    int testsPassedValue = 0;
    bool nextEffect = true;
    auto jTestResultAddtoArray = [&]( const bool resultValue ) {
        Json jItem;
        jItem[ "name" ] = currentEffect;
        jItem[ "result" ] = resultValue;
        jSubTestsResults.push_back( jItem );
    };

    auto timeout = m_timeout;
    auto effectStartTime = TimeUtils::getMonotonicTime();
    while( true ) {
        if( nextEffect ) {
            nextEffect = false;

            auto effect = testEffects.front();
            currentEffect = effect.first;
            testLog() << "Running " << currentEffect << " test effect - press recovery button to commit...";

            m_emotionLed.setColor( effect.second );
            m_emotionLed.turnBlinking( 125, 125 );
            effectStartTime = TimeUtils::getMonotonicTime();
            testEffects.erase( testEffects.begin() );
        }

        if( buttonPressed() ) {
            timeout = m_defaultTimeout;
            jTestResultAddtoArray( true );
            indicateResult( true, currentEffect );
            if( result != TestResult::FAILED ) {
                result = TestResult::DONE;
            }
            ++testsPassedValue;
            nextEffect = !testEffects.empty();
            if( !nextEffect ) {
                break;
            }
        }
        else if( TimeUtils::getDiffMonotonicTime( effectStartTime ) >= timeout ) {
            timeout = m_defaultTimeout;
            jTestResultAddtoArray( false );
            indicateResult( false, currentEffect );
            result = TestResult::FAILED;
            nextEffect = !testEffects.empty();
            if( !nextEffect ) {
                break;
            }
        }
    }
    m_emotionLed.turnOff();
    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    Json jTestResult;
    jTestResult[ "name" ] = m_name;
    jTestResult[ "result" ] = result == TestResult::DONE ? true : false;
    jTestResult[ "threshold" ] = testsPassThreshold;
    jTestResult[ "value" ] = testsPassedValue;
    jTestResult[ "subtests" ] = jSubTestsResults;
    testLog() << getTestResultJsonReport( jTestResult );

    // TODO make this pretty somehow
    const bool buttonPressed = ( testsPassedValue > 0 );
    jTestResult[ "name" ] = "Button";
    jTestResult[ "result" ] = buttonPressed;
    jTestResult[ "threshold" ] = 1;
    jTestResult[ "value" ] = testsPassedValue;
    testLog() << getTestResultJsonReport( "Button", jTestResult );

    return result;
}

bool EmotionLedTest::buttonPressed() {
    m_previousButtonState = m_currentButtonState;
    m_currentButtonState = m_button.getState();
    return ( m_currentButtonState != m_previousButtonState && m_currentButtonState == common::GpioState::OFF );
}

void EmotionLedTest::indicateResult( const bool result, const std::string& effectName ) {
    testLog() << getTestResultReport( std::string( m_name ).append( " test effect: " ).append( effectName ),
                                      result ? TestResult::DONE : TestResult::FAILED );
}

void EmotionLedTest::setUp( const ArgParser& argParser ) {
    try {
        const std::string argLedTimeout = "--led-timeout";

        if( argParser.hasArg( argLedTimeout ) ) {
            std::string timeoutStr = argParser.getValue( argLedTimeout );
            if( !ArgUtils::isNumber( timeoutStr ) )
                throw ArgumentError( "LED timeout is not a number" );
            m_timeout = std::stoul( timeoutStr );
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
