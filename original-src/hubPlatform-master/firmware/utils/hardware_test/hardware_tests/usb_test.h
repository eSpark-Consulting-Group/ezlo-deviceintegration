#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"

namespace HubPlatform {
namespace HardwareTest {

class UsbTest final : public HardwareTestBase {
public:
    UsbTest() = delete;
    explicit UsbTest( const volatile sig_atomic_t& exitSignalRef );
    ~UsbTest() = default;

public:
    TestResult run() override;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
