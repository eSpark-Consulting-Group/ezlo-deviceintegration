#pragma once

#include "hardware_test/hardware_tests/emotion_led_test.h"
#include "hardware_test/hardware_tests/battery_test.h"
#include "hardware_test/hardware_tests/sound_test.h"
#include "hardware_test/hardware_tests/recovery_button_test.h"
#include "hardware_test/hardware_tests/zwave_test.h"
#include "hardware_test/hardware_tests/connection_test.h"
#include "hardware_test/hardware_tests/usb_test.h"
#include "hardware_test/hardware_tests/zigbee_test.h"
#include "hardware_test/hardware_tests/bluetooth_test.h"
#include "hardware_test/hardware_tests/lte_test.h"
#include "hardware_test/task_composition.h"

#include <csignal>
#include <vector>

namespace HubPlatform {
namespace HardwareTest {

class ArgParser;

class HardwareTester final {
public:
    HardwareTester( const volatile sig_atomic_t& exitSignalRef );
    ~HardwareTester() = default;

public:
    void setUp( const std::vector< std::string >& args );
    int runTestLoop();

private:
    void printUniqueData();
    void setUpTester( const ArgParser& argParser );
    void setupSupportedTests( const ArgParser& argParser );
    std::unique_ptr< TaskQueue > createTaskQueue( const std::vector< HardwareTestBase* >& tests,
                                                  TaskQueue::Order order );

private:
    const volatile sig_atomic_t& m_exitSignal;

    EmotionLedTest m_emotionLedTest;
    SoundTest m_soundTest;
    BatteryTest m_batteryTest;
    UsbTest m_usbTest;
    RecoveryButtonTest m_recoveryButtonTest;
    ZwaveTest m_zwaveTest;
    ZigbeeTest m_zigbeeTest;
    BluetoothTest m_bluetoothTest;
    LteTest m_lteTest;
    ConnectionTest m_connectionTest;

    std::vector< HardwareTestBase* > m_allTests;
    std::vector< HardwareTestBase* > m_tests;
    TaskQueue::Order m_execOrder{ TaskQueue::Order::Serial };
};

}  // namespace HardwareTest
}  // namespace HubPlatform
