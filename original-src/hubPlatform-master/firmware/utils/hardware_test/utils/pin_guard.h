#pragma once

namespace common {
class Gpio;
}  // namespace common

namespace HubPlatform {
namespace HardwareTest {

class PinGuard {
public:
    PinGuard( common::Gpio& pin );
    ~PinGuard();

private:
    common::Gpio& m_pin;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
