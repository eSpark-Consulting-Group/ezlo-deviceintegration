#include "hardware_test/hardware_tests/battery_test.h"

#if defined( __HUB_TYPE_H2_SECURE__ )

#include "hardware_manager/battery/bq27441g1/bq27441g1.h"
#include "base_utils/timeutils.h"
#include "base_utils/utils.h"
#include "base_utils/gpio.h"
#include "base_utils/uart/uart.h"
#include "common_utils/cpp14.h"
#include "system_utils/system.h"
#include "hardware_test/utils/test_log.h"
#include "hardware_test/utils/lte_module.h"

#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <regex>

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "Battery"
constexpr std::chrono::seconds TEST_TIMEOUT{ 130 };

namespace {
TestLog testLog( TEST_NAME );

constexpr const char* BatteryAttachRequestPhrase = "Waiting for battery attach...";
constexpr const char* BatteryDetachRequestPhrase = "Waiting for battery detach...";
constexpr const char* PowerAdapterAttachRequestPhrase = "Waiting for power adapter attach...";
constexpr const char* PowerAdapterDetachRequestPhrase = "Waiting for power adapter detach...";

constexpr const auto Timeout = std::chrono::seconds( 10 );
constexpr const auto CheckInterval = std::chrono::milliseconds( 250 );

bool inRange( int min, int max, int value ) {
    return ( value >= min && value <= max );
};

class Gpio {
public:
    explicit Gpio( int gpio, bool activeHigh = false )
    : m_gpio( gpio )
    , m_activeHigh( activeHigh ) {}

    ~Gpio() {
        if( m_fd == -1 )
            return;

        close( m_fd );
    }

    bool open() {
        m_fd = ::open( ( "/sys/class/gpio/gpio" + std::to_string( m_gpio ) + "/value" ).c_str(), O_WRONLY );
        return ( m_fd != -1 );
    }

    bool on() {
        if( m_fd == -1 )
            return false;

        return ( write( m_fd, m_activeHigh ? "1" : "0", 1 ) != 1 );
    }

    bool off() {
        if( m_fd == -1 )
            return false;

        return ( write( m_fd, m_activeHigh ? "0" : "1", 1 ) != 1 );
    }

private:
    int m_fd;
    int m_gpio;
    bool m_activeHigh;
};

bool hasBatteryFuelGaugeMeter() {
    const uint8_t i2cBusNum = 1;
    const uint16_t gaugeMeterDeviceAddress = 0x55;
    int result;

    std::tie( std::ignore, result ) = System::getCommandOutput(
        ( "i2cget -y " + std::to_string( i2cBusNum ) + " " + std::to_string( gaugeMeterDeviceAddress ) ).c_str() );
    return result == 0;
}

}  // namespace

class BatteryGaugeMeter : public BatteryTest::MeterBase {
public:
    MeterType getMeterType() override {
        return MeterType::BatteryGauge;
    }

    bool isBatteryAttached() override {
        return m_gauge.ensureOpen();
    }

    int getVoltage() override {
        return checked( m_gauge.readVoltage() );
    }

    int getCurrent() override {
        return checked( m_gauge.readAverageCurrent() );
    }

private:
    int checked( int value ) {
        if( value == -1 || value == 0xFFFF )
            throw HardwareManager::Bq27441g1::CommunicationError( cc::error( "Read value is bad" ) );

        return value;
    };

private:
    HardwareManager::Bq27441g1::Bq27441g1 m_gauge;
};

class ModemADCMeter : public BatteryTest::MeterBase {
public:
    ModemADCMeter( TestLog& logger )
    : m_lteModule( logger ) {
        // NOTE: Modem requires 5s to accumulate ADC value. Prior to this time it will report an error.
        std::chrono::seconds modemWarmUpTime{ 7 };
        m_lteModule.wakeUp( modemWarmUpTime );
    }

    MeterType getMeterType() override {
        return MeterType::ModemADC;
    }

    bool isBatteryAttached() override {
        // empirical value, if battery is missing ADC returns values below this value
        return m_lteModule.getADCValue() > 500.0;
    }

    int getVoltage() override {
        return m_lteModule.getADCValue();
    }

    int getCurrent() override {
        return 0;  // NOTE: not available for modem ADC Meter
    }

private:
    LteModule m_lteModule;
};

BatteryTest::BatteryTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef ) {}

HardwareTestInterface::TestResult BatteryTest::run() {
    auto make_test = []( const char* name ) { return JSON( "name" << name << "result" << false ); };
    auto pass = []( JsonObject& j ) { j[ "result" ] = true; };
    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;

    auto testBatteryAttach = make_test( "battery attach" );
    auto testPowerAdapterDetach = make_test( "power adapter detach" );
    auto testPowerAdapterAttach = make_test( "power adapter attach" );
    auto testPowerAdapterDisable = make_test( "power adapter disable" );
    auto testBatteryDetach = make_test( "battery detach" );
    auto testResults = make_test( m_name.c_str() );

    Gpio charger( 352 );
    testLog() << "Starting " << m_name << " test...";
    auto startTestTime = TimeUtils::getMonotonicTime();
    try {
        initTest();

        bool ok;
        int voltage = 0;
        int current = 0;

        if( !charger.open() ) {
            // Expecting charger enable gpio is already exported and proper direction set
            throw std::runtime_error( "Unable to open charger enable gpio" );
        }
        testLog() << "Disabling charger...";
        charger.off();
        testLog() << BatteryAttachRequestPhrase;
        if( !waitForCondition( [&]() { return m_meter->isBatteryAttached(); }, Timeout, CheckInterval ) ) {
            throw std::runtime_error( "Timeout waiting for battery attach: battery is missing" );
        }

        testLog() << "Detected battery attached";
        testLog() << "Enabling charger...";
        charger.on();

        testLog() << "Waiting for battery charging...";
        std::tie( ok, voltage, current ) =
            waitForVoltageCurrent( m_MinChargeVoltage, m_MaxChargeVoltage, m_MinChargeCurrent, m_MaxChargeCurrent );
        testBatteryAttach[ "voltage" ] = voltage;
        testBatteryAttach[ "current" ] = current;
        if( !ok ) {
            throw std::runtime_error( "Timeout waiting for battery charging" );
        }
        pass( testBatteryAttach );

        testLog() << PowerAdapterDetachRequestPhrase;
        std::tie( ok, voltage, current ) = waitForVoltageCurrent(
            m_MinDischargeVoltage, m_MaxDischargeVoltage, m_MinDischargeCurrent, m_MaxDischargeCurrent );
        testPowerAdapterDetach[ "voltage" ] = voltage;
        testPowerAdapterDetach[ "current" ] = current;
        if( !ok ) {
            throw std::runtime_error( "Timeout waiting for battery discharging" );
        }
        pass( testPowerAdapterDetach );

        testLog() << PowerAdapterAttachRequestPhrase;
        std::tie( ok, voltage, current ) =
            waitForVoltageCurrent( m_MinChargeVoltage, m_MaxChargeVoltage, m_MinChargeCurrent, m_MaxChargeCurrent );
        testPowerAdapterAttach[ "voltage" ] = voltage;
        testPowerAdapterAttach[ "current" ] = current;
        if( !ok ) {
            throw std::runtime_error( "Timeout waiting for battery charging" );
        }
        pass( testPowerAdapterAttach );

        testLog() << "Disabling charger...";
        charger.off();
        std::tie( ok, voltage, current ) =
            waitForVoltageCurrent( m_MinIdleVoltage, m_MaxIdleVoltage, m_MinIdleCurrent, m_MaxIdleCurrent );
        testPowerAdapterDisable[ "voltage" ] = voltage;
        testPowerAdapterDisable[ "current" ] = current;
        if( !ok ) {
            throw std::runtime_error( "Timeout waiting for idle mode" );
        }
        pass( testPowerAdapterDisable );

        testLog() << BatteryDetachRequestPhrase;
        if( !waitForCondition( [&]() { return !m_meter->isBatteryAttached(); }, Timeout, CheckInterval ) ) {
            throw std::runtime_error( "Timeout waiting for battery detach: battery is still attached" );
        }
        testLog() << "Detected battery detached";
        pass( testBatteryDetach );

        result = TestResult::DONE;
    }
    catch( const HardwareManager::Bq27441g1::CommunicationError& error ) {
        result = TestResult::FAILED;
        testLog() << "\n"
                  << "Can't communicate with gauge: " << error.what();
    }
    catch( std::runtime_error& e ) {
        result = TestResult::FAILED;
        testLog() << e.what();
    }

    testLog() << "Enabling charger...";
    charger.on();

    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    testResults[ "result" ] = ( result == TestResult::DONE ) ? true : false;
    JsonArray subtests = {
        testBatteryAttach, testPowerAdapterDetach, testPowerAdapterAttach, testPowerAdapterDisable, testBatteryDetach
    };
    size_t passed = std::count_if( subtests.begin(), subtests.end(), []( JsonObject& j ) { return j[ "result" ]; } );
    testResults[ "value" ] = passed;
    testResults[ "threshold" ] = subtests.size();
    testResults[ "subtests" ] = std::move( subtests );

    testLog() << getTestResultJsonReport( testResults );

    return result;
}

void BatteryTest::initTest() {
    testLog() << "Init battery test";

    if( hasBatteryFuelGaugeMeter() ) {
        m_meter = std::make_unique< BatteryGaugeMeter >();
        testLog() << "Battery gauge is used as a meter";

        m_MinChargeVoltage = 3300;
        m_MaxChargeVoltage = 4000;
        m_MinChargeCurrent = 800;
        m_MaxChargeCurrent = 1500;

        m_MinIdleVoltage = 3100;
        m_MaxIdleVoltage = 3900;
        m_MinIdleCurrent = 0;
        m_MaxIdleCurrent = 0;

        m_MinDischargeVoltage = 3100;
        m_MaxDischargeVoltage = 3900;
        m_MinDischargeCurrent = -1000;
        m_MaxDischargeCurrent = -100;
    }
    else {
        m_meter = std::make_unique< ModemADCMeter >( testLog );
        testLog() << "Modem ADC is used as a meter";

        m_MinChargeVoltage = 1650;
        m_MaxChargeVoltage = 2100;
        m_MinChargeCurrent = 0;
        m_MaxChargeCurrent = 0;

        m_MinIdleVoltage = 1550;
        m_MaxIdleVoltage = 2000;
        m_MinIdleCurrent = 0;
        m_MaxIdleCurrent = 0;

        m_MinDischargeVoltage = 1550;
        m_MaxDischargeVoltage = 2000;
        m_MinDischargeCurrent = 0;
        m_MaxDischargeCurrent = 0;
    }
}

std::tuple< bool, int, int > BatteryTest::waitForVoltageCurrent( int minExpectedVoltage,
                                                                 int maxExpectedVoltage,
                                                                 int minExpectedCurrent,
                                                                 int maxExpectedCurrent ) {
    testLog() << "Expecting V in " << minExpectedVoltage << ".." << maxExpectedVoltage << " mV, I in "
              << minExpectedCurrent << ".." << maxExpectedCurrent << " mA";

    int voltage = 0;
    int current = 0;

    bool ok = waitForCondition(
        [&]() {
            if( m_meter->getMeterType() == MeterBase::MeterType::BatteryGauge ) {
                current = m_meter->getCurrent();
                voltage = m_meter->getVoltage();

                testLog() << "Detected V = " << voltage << " mV, I = " << current << " mA";

                return inRange( minExpectedVoltage, maxExpectedVoltage, voltage ) &&
                       inRange( minExpectedCurrent, maxExpectedCurrent, current );
            }
            else {
                voltage = m_meter->getVoltage();
                testLog() << "Detected V = " << voltage << " mV, I = NA mA";

                return inRange( minExpectedVoltage, maxExpectedVoltage, voltage );
            }
        },
        Timeout,
        CheckInterval );

    return std::make_tuple( ok, voltage, current );
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#else

namespace HubPlatform {
namespace HardwareTest {

BatteryTest::BatteryTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( "DummyBattery", std::chrono::seconds( 1 ), exitSignalRef ) {}

HardwareTestInterface::TestResult BatteryTest::run() {
    return HardwareTestInterface::TestResult::DONE;
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#endif
