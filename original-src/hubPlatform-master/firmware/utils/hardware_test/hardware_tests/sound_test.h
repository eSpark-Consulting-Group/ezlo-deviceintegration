#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"
#include "hardware_test/arg_parser.h"

namespace HubPlatform {
namespace HardwareTest {

class SoundTest final : public HardwareTestBase {
public:
    SoundTest() = delete;
    explicit SoundTest( const volatile sig_atomic_t& exitSignalRef );
    ~SoundTest() = default;

public:
    TestResult run() override;
    void setUp( const ArgParser& argParser );

private:
    std::chrono::seconds m_wait_timeout{ 10 };
};

}  // namespace HardwareTest
}  // namespace HubPlatform
