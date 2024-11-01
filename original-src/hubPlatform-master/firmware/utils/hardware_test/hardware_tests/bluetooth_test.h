#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"

namespace HubPlatform {
namespace HardwareTest {

class BluetoothTest final : public HardwareTestBase {
public:
    BluetoothTest() = delete;
    explicit BluetoothTest( const volatile sig_atomic_t& exitSignalRef );
    ~BluetoothTest() = default;

public:
    TestResult run() override;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
