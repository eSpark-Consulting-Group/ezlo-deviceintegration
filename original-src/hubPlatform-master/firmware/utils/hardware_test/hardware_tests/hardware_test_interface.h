#pragma once

#include <unordered_map>

namespace HubPlatform {
namespace HardwareTest {

class HardwareTestInterface {
public:
    enum TestResult : uint8_t { FAILED = 0, UNKNOWN = 254, DONE = 255 };

public:
    HardwareTestInterface() = default;
    virtual ~HardwareTestInterface() = default;

public:
    virtual TestResult run() = 0;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
