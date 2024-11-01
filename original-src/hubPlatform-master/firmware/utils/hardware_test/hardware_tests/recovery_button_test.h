#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"

#include "config_reader/config_reader.h"
#include "hardware_manager/config/hw_manager_config.h"

namespace HubPlatform {
namespace HardwareTest {

class RecoveryButtonTest final : public HardwareTestBase {
public:
    RecoveryButtonTest() = delete;
    explicit RecoveryButtonTest( const volatile sig_atomic_t& exitSignalRef );
    ~RecoveryButtonTest() = default;

public:
    TestResult run() override;

private:
    static constexpr time_t buttonTestTimeoutSeconds = 20;

private:
    HardwareManager::HWManagerConfig m_config;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
