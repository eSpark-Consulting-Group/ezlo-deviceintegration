#pragma once
#include "hardware_manager/button/reset_button_base.h"
#include "hardware_manager/button/generic/gpio_button_state.h"

namespace HubPlatform {
namespace HardwareManager {

class GenericResetButton : public ResetButtonBase {
public:
    GenericResetButton( OnCombinationExecuted handler, int gpioNumber );

protected:
    ButtonState& getButtonState() override {
        return m_buttonState;
    }

protected:
    GpioButtonState m_buttonState;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
