#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareTest {

class ZigbeeTest final : public HardwareTestBase {
public:
    ZigbeeTest() = delete;
    explicit ZigbeeTest( const volatile sig_atomic_t& exitSignalRef );
    ~ZigbeeTest() = default;
    JsonObject getUniqueEUI64();

public:
    TestResult run() override;

private:
    JsonObject m_uniqueEUI64;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
