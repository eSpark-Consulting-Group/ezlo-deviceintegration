#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"

#include "config_reader/config_reader.h"
#include "hardware_manager/config/hw_manager_config.h"
#include "hardware_manager/button/reset_button.h"
#include "base_utils/uart/uart.h"
#include "base_utils/zwave/zwave_stick.h"
#include "base_utils/zwave/zwave_serial_api.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareTest {

class ZwaveTest final : public HardwareTestBase {
public:
    ZwaveTest() = delete;
    explicit ZwaveTest( const volatile sig_atomic_t& exitSignalRef );
    ~ZwaveTest() = default;
    JsonObject getUniqueDSK();

public:
    TestResult run() override;

private:
    TestResult testEnvm( Json& jTestResults );
    TestResult testSerialApi( Json& jTestResults );

private:
    HardwareManager::HWManagerConfig m_config;
    JsonObject m_uniqueDSK;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
