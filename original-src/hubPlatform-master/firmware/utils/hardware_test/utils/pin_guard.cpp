#include "hardware_test/utils/pin_guard.h"

#include "base_utils/gpio.h"

namespace HubPlatform {
namespace HardwareTest {

PinGuard::PinGuard( common::Gpio& pin )
: m_pin( pin ) {
    m_pin.setState( common::GpioState::ON );
};

PinGuard::~PinGuard() {
    m_pin.setState( common::GpioState::OFF );
};

}  // namespace HardwareTest
}  // namespace HubPlatform
