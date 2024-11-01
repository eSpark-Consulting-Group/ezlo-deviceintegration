#include "hardware_test/hardware_tests/sound_test.h"

#if defined( __HUB_TYPE_H2_SECURE__ )

#include "hardware_test/utils/test_log.h"

#include "hardware_manager/config/hw_manager_config.h"
#include "base_utils/timeutils.h"
#include "base_utils/utils.h"
#include "base_utils/gpio.h"
#include "system_utils/system.h"
#include "system_utils/filesystem.h"

#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "Sound"
constexpr std::chrono::seconds TEST_TIMEOUT{ 80 };

namespace {
TestLog testLog( TEST_NAME );

// Recording volume
constexpr const int Mic1BoostVolume = 7;       // 0 - 7
constexpr const int ADCGainCaptureVolume = 7;  // 0 - 7

// Playback volume
constexpr const int LineOutPlaybackVolume = 31;  // 0 - 31
constexpr const int DACPlaybackVolume = 63;      // 0 - 63

// Recording quality
constexpr const int SamplingRate = 48000;  // 44100 or 48000
constexpr const char SoundFile[] = "/tmp/sound_test.raw";

constexpr const auto Timeout = std::chrono::seconds( 10 );
constexpr const auto CheckInterval = std::chrono::milliseconds( 50 );

constexpr const int ButtonGpio = 67;

bool waitForGpio( const common::Gpio& gpio, common::GpioState state, std::chrono::seconds timeout = Timeout ) {
    return waitForCondition( [&]() { return ( gpio.getState() == state ); }, timeout, CheckInterval );
}

bool amixerSet( const std::string& name, int value ) {
    return ::system( ( "amixer cset name='" + name + "' " + std::to_string( value ) + " > /dev/null" ).c_str() ) == 0;
}

bool amixerSet( const std::string& name ) {
    return ::system( ( "amixer cset name='" + name + "' on >> /dev/null" ).c_str() ) == 0;
}

bool configureAmixer() {
    bool ok = true;
    ok &= amixerSet( "Line Out Playback Switch" );
    ok &= amixerSet( "Line Out Playback Volume", LineOutPlaybackVolume );
    ok &= amixerSet( "DAC Playback Switch" );
    ok &= amixerSet( "DAC Playback Volume", DACPlaybackVolume );
    ok &= amixerSet( "Mic1 Capture Switch" );
    ok &= amixerSet( "Mic1 Boost Volume", Mic1BoostVolume );
    ok &= amixerSet( "ADC Gain Capture Volume", ADCGainCaptureVolume );
    return ok;
}

void writeSilence32( std::ofstream& ofs, int milliseconds ) {
    for( int i = 0; i < SamplingRate * milliseconds / 1000; ++i ) {
        int32_t sample = 0;
        ofs.write( reinterpret_cast< const char* >( &sample ), sizeof( sample ) );
    }
}

void writeSin32( std::ofstream& ofs, int milliseconds, double frequency, int harmonics ) {
    int32_t amplitude = 20.0 / ( harmonics + 1 );
    for( int i = 0; i < SamplingRate * milliseconds / 1000; ++i ) {
        int32_t sample = 0;
        int32_t freq = frequency;
        for( int j = 0; j < harmonics + 1; ++j ) {
            sample += amplitude * sin( 2.0 * M_PI * freq * i / SamplingRate );
            freq *= 2;
        }
        sample = ( ( ( sample & 0xff000000 ) >> 24 ) | ( ( sample & 0x00ff0000 ) >> 8 ) |
                   ( ( sample & 0x0000ff00 ) << 8 ) | ( ( sample & 0x000000ff ) << 24 ) );
        ofs.write( reinterpret_cast< const char* >( &sample ), sizeof( sample ) );
    }
}

int startRecording() {
    const auto pid = ::fork();
    if( pid == 0 ) {
        std::ofstream ofs( SoundFile, std::ofstream::binary );
        // Write silence to wake up sound card first
        writeSilence32( ofs, 1200 );
        // Write start sound
        writeSin32( ofs, 100, 800, 4 );
        writeSin32( ofs, 100, 1000, 4 );
        ofs.close();

        std::vector< const char* > args = {
            "arecord", "-q", "-r", std::to_string( SamplingRate ).c_str(), "-t", "raw", "-f", "S32_LE", nullptr
        };
        int fd = open( SoundFile, O_WRONLY | O_APPEND );
        dup2( fd, 1 );
        close( fd );
        ::execvp( args.front(), (char**) args.data() );
        return 0;
    }
    else {
        return pid;
    }
}

bool stopRecording( int pid ) {
    return (::kill( pid, SIGTERM ) == 0 );
}

bool playback() {
    // Write ending sound
    std::ofstream ofs( SoundFile, std::ofstream::binary | std::ofstream::app );
    writeSin32( ofs, 100, 1000, 4 );
    writeSin32( ofs, 100, 800, 4 );
    ofs.close();

    return ( System::getCommandOutput(
                 ( "aplay -r " + std::to_string( SamplingRate ) + " -t raw -f S32_LE " + SoundFile ).c_str() )
                 .second == 0 );
}

}  // namespace

SoundTest::SoundTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef ) {}

HardwareTestInterface::TestResult SoundTest::run() {
    auto make_test = []( const char* name ) { return JSON( "name" << name << "result" << false ); };
    auto pass = []( JsonObject& j ) { j[ "result" ] = true; };
    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;

    auto testSoundRecording = make_test( "Sound recording" );
    auto testSoundPlayback = make_test( "Sound playback" );
    auto testResults = make_test( m_name.c_str() );

    auto hardwareConfig = HardwareManager::HWManagerConfig();
    auto& emotionLed = hardwareConfig.getEmotionLed();

    testLog() << "Starting " << m_name << " test...";
    auto startTestTime = TimeUtils::getMonotonicTime();
    try {
        common::Gpio button( ButtonGpio );
        button.setDirection( common::GpioDirection::IN );
        if( !configureAmixer() ) {
            throw std::runtime_error( "Failed to configure amixer" );
        }

        emotionLed.setColor( Rgb::Magenta );
        emotionLed.turnBlinking( 125, 125 );
        testLog() << "Waiting for button pressed to start recording...";
        if( !waitForGpio( button, common::GpioState::OFF, m_wait_timeout ) ) {
            throw std::runtime_error( "Timeout waiting for button pressed" );
        }
        emotionLed.setColor( Rgb::Red );
        emotionLed.turnOn();
        testLog() << "Starting recording (max "
                  << (int) std::chrono::duration_cast< std::chrono::seconds >( Timeout ).count() << "s)";
        int pid = startRecording();
        if( pid == 0 ) {
            throw std::runtime_error( "Failed to start recording" );
        }
        // NOTE: maybe we need enforce some minimum recording time
        testLog() << "Waiting for button depressed to stop recording...";
        if( !waitForGpio( button, common::GpioState::ON ) ) {
            stopRecording( pid );
            throw std::runtime_error( "Timeout waiting for button depressed" );
        }
        testLog() << "Stopping recording...";
        if( !stopRecording( pid ) ) {
            throw std::runtime_error( "Failed to stop recording" );
        }
        pass( testSoundRecording );

        emotionLed.setColor( Rgb::Green );
        emotionLed.turnOn();
        testLog() << "Starting playback...";
        if( !playback() ) {
            throw std::runtime_error( "Failed starting playback" );
        }
        testLog() << "Playback stopped";
        emotionLed.setColor( Rgb::Yellow );
        emotionLed.turnBlinking( 125, 125 );
        testLog() << "Waiting for button press if playback was correct...";
        if( !waitForGpio( button, common::GpioState::OFF ) ) {
            throw std::runtime_error( "Timeout waiting for button pressed" );
        }
        pass( testSoundPlayback );

        result = TestResult::DONE;
    }
    catch( std::runtime_error& e ) {
        result = TestResult::FAILED;
        testLog() << e.what();
    }
    emotionLed.turnOff();
    filesystem::removeFile( SoundFile );

    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    testResults[ "result" ] = ( result == TestResult::DONE ) ? true : false;
    JsonArray subtests = { testSoundRecording, testSoundPlayback };
    size_t passed = std::count_if( subtests.begin(), subtests.end(), []( JsonObject& j ) { return j[ "result" ]; } );
    testResults[ "value" ] = passed;
    testResults[ "threshold" ] = subtests.size();
    testResults[ "subtests" ] = std::move( subtests );

    testLog() << getTestResultJsonReport( testResults );

    return result;
}

void SoundTest::setUp( const ArgParser& argParser ) {
    try {
        const std::string argSoundTimeout = "--sound-timeout";

        if( argParser.hasArg( argSoundTimeout ) ) {
            std::string timeoutStr = argParser.getValue( argSoundTimeout );
            if( !ArgUtils::isNumber( timeoutStr ) )
                throw ArgumentError( "Sound timeout is not a number" );
            m_wait_timeout = std::chrono::seconds( std::stoul( timeoutStr ) );
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

#else

namespace HubPlatform {
namespace HardwareTest {

SoundTest::SoundTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( "DummySound", std::chrono::seconds( 1 ), exitSignalRef ) {}

HardwareTestInterface::TestResult SoundTest::run() {
    return HardwareTestInterface::TestResult::DONE;
}

void SoundTest::setUp( const ArgParser& argParser ) {}

}  // namespace HardwareTest
}  // namespace HubPlatform

#endif
