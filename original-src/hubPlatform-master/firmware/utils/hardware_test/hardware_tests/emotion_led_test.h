#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"

#include "config_reader/config_reader.h"
#include "hardware_manager/config/hw_manager_config.h"
#include "hardware_manager/effect/effect_handler.h"
#include "hardware_manager/effect/effects.h"
#include "hardware_manager/led/led.h"
#include "base_utils/prioritized_task_pool.h"
#include "base_utils/gpio.h"

#include "hardware_test/arg_parser.h"

namespace HubPlatform {
namespace HardwareTest {

class EmotionLedTest final : public HardwareTestBase {
public:
    EmotionLedTest() = delete;
    explicit EmotionLedTest( const volatile sig_atomic_t& exitSignalRef );
    ~EmotionLedTest() = default;

public:
    TestResult run() override;
    void setUp( const ArgParser& argParser );

private:
    bool buttonPressed();
    void indicateResult( const bool result, const std::string& effectName );

private:
    HardwareManager::HWManagerConfig m_config;
    LedColor& m_emotionLed;
    common::Gpio m_button;
    common::GpioState m_previousButtonState;
    common::GpioState m_currentButtonState;
    size_t m_defaultTimeout = 7;
    size_t m_timeout = 7;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
